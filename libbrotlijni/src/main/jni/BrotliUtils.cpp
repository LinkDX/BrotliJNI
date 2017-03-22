//
// Created by LukeSLLin on 2017/2/17.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <brotli/decode.h>
#include <brotli/encode.h>
#include <dirent.h>
#include <errno.h>

#include <unistd.h>
#include <utime.h>

#include "com_fihtdc_libbrotlijni_BrotliUtils.h"
#include "BrotliUtils.h"

#define MAKE_BINARY(FILENO) (_setmode((FILENO), _O_BINARY), (FILENO))

static FILE* OpenInputFile(const char* input_path) {
    FILE* fp = NULL;
    if (input_path == 0) {
        return fdopen(STDIN_FILENO, "rb");
    }
    fp = fopen(input_path, "rb");
    if (fp == NULL) {
        LOGE("OpenInputFile() Open file error:%s", strerror(errno));
    }
    return fp;
}

static FILE *OpenOutputFile(const char *output_path, const int force) {
    int fd;
    if (output_path == 0) {
        return fdopen(STDOUT_FILENO, "wb");
    }
    fd = open(output_path, O_CREAT | (force ? 0 : O_EXCL) | O_WRONLY | O_TRUNC,
            S_IRUSR | S_IWUSR);
    if (fd < 0) {
        if (!force) {
            struct stat statbuf;
            if (stat(output_path, &statbuf) == 0) {
                LOGE("OpenOutputFile() File not exist");
            }
        }
        LOGE("OpenOutputFile() Open file error:%s", strerror(errno));
    }
    return fdopen(fd, "wb");
}

static int64_t FileSize(const char *path) {
    FILE *f = fopen(path, "rb");
    int64_t retval;
    if (f == NULL) {
        return -1;
    }
    if (fseek(f, 0L, SEEK_END) != 0) {
    fclose(f);
        return -1;
    }
    retval = ftell(f);
    if (fclose(f) != 0) {
        return -1;
    }
    return retval;
}

/* Result ownersip is passed to caller.
   |*dictionary_size| is set to resulting buffer size. */
static uint8_t* ReadDictionary(const char* path, size_t* dictionary_size) {
    static const int kMaxDictionarySize = (1 << 24) - 16;
    FILE *f = fopen(path, "rb");
    int64_t file_size_64;
    uint8_t* buffer;
    size_t bytes_read;

    if (f == NULL) {
        LOGE("ReadDictionary() Open file error:%s", strerror(errno));
        return 0;
    }

    file_size_64 = FileSize(path);
    if (file_size_64 == -1) {
        LOGE("ReadDictionary() Could not get size of dictionary file");
        return 0;
    }

    if (file_size_64 > kMaxDictionarySize) {
        LOGE("ReadDictionary() Dictionary is larger than maximum allowed: %d",
                kMaxDictionarySize);
        return 0;
    }
    *dictionary_size = (size_t)file_size_64;

    buffer = (uint8_t*)malloc(*dictionary_size);
    if (!buffer) {
        LOGE("ReadDictionary() Could not read dictionary: out of memory");
        return 0;
    }
    bytes_read = fread(buffer, sizeof(uint8_t), *dictionary_size, f);
    if (bytes_read != *dictionary_size) {
        LOGE("ReadDictionary() Could not read dictionary");
        return 0;
    }
    fclose(f);
    return buffer;
}

static const size_t kFileBufferSize = 65536;

static int Decompress(FILE* fin, FILE* fout, const char* dictionary_path) {
    /* Dictionary should be kept during first rounds of decompression. */
    uint8_t* dictionary = NULL;
    uint8_t* input;
    uint8_t* output;
    size_t available_in;
    const uint8_t* next_in;
    size_t available_out = kFileBufferSize;
    uint8_t* next_out;
    BrotliDecoderResult result = BROTLI_DECODER_RESULT_ERROR;
    BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!s) {
        LOGE("Decompress() Out of memory");
        return 0;
    }
    if (dictionary_path != NULL) {
        size_t dictionary_size = 0;
        dictionary = ReadDictionary(dictionary_path, &dictionary_size);
        if(dictionary != 0){
            BrotliDecoderSetCustomDictionary(s, dictionary_size, dictionary);
        }
    }
    input = (uint8_t*)malloc(kFileBufferSize);
    output = (uint8_t*)malloc(kFileBufferSize);
    if (!input || !output) {
        LOGE("Decompress() Out of memory");
        goto end;
    }
    next_out = output;
    result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
    while (1) {
    if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
        if (feof(fin)) {
            break;
        }
        available_in = fread(input, 1, kFileBufferSize, fin);
        next_in = input;
        if (ferror(fin)) {
            break;
        }
    } else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
        fwrite(output, 1, kFileBufferSize, fout);
        if (ferror(fout)) {
            break;
        }
        available_out = kFileBufferSize;
        next_out = output;
    } else {
        break; /* Error or success. */
    }
    result = BrotliDecoderDecompressStream(
        s, &available_in, &next_in, &available_out, &next_out, 0);
    }
    if (next_out != output) {
        fwrite(output, 1, (size_t)(next_out - output), fout);
    }

    if ((result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) || ferror(fout)) {
        LOGE("failed to write output");
    } else if (result != BROTLI_DECODER_RESULT_SUCCESS) {
    /* Error or needs more input. */
        LOGE("corrupt input");
    }

end:
    free(dictionary);
    free(input);
    free(output);
    BrotliDecoderDestroyInstance(s);
    return (result == BROTLI_DECODER_RESULT_SUCCESS) ? 1 : 0;
}


static int Compress(int quality, int lgwin, FILE* fin, FILE* fout,
    const char *dictionary_path) {
    BrotliEncoderState* s = BrotliEncoderCreateInstance(0, 0, 0);
    uint8_t* buffer = (uint8_t*)malloc(kFileBufferSize << 1);
    uint8_t* input = buffer;
    uint8_t* output = buffer + kFileBufferSize;
    size_t available_in = 0;
    const uint8_t* next_in = NULL;
    size_t available_out = kFileBufferSize;
    uint8_t* next_out = output;
    int is_eof = 0;
    int is_ok = 1;

    if (!s || !buffer) {
        is_ok = 0;
        goto finish;
    }

    BrotliEncoderSetParameter(s, BROTLI_PARAM_QUALITY, (uint32_t)quality);
    BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, (uint32_t)lgwin);
    if (dictionary_path != NULL) {
        size_t dictionary_size = 0;
        uint8_t* dictionary = ReadDictionary(dictionary_path, &dictionary_size);
        if(dictionary!=0){
            BrotliEncoderSetCustomDictionary(s, dictionary_size, dictionary);
            free(dictionary);
        }
    }

    while (1) {
        if (available_in == 0 && !is_eof) {
            available_in = fread(input, 1, kFileBufferSize, fin);
            next_in = input;
            if (ferror(fin)) break;
            is_eof = feof(fin);
        }

        if (!BrotliEncoderCompressStream(s,
            is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS,
            &available_in, &next_in, &available_out, &next_out, NULL)) {
            is_ok = 0;
            break;
        }

        if (available_out != kFileBufferSize) {
            size_t out_size = kFileBufferSize - available_out;
            fwrite(output, 1, out_size, fout);
            if (ferror(fout)) break;
            available_out = kFileBufferSize;
            next_out = output;
        }

        if (BrotliEncoderIsFinished(s)) break;
    }

finish:
    free(buffer);
    BrotliEncoderDestroyInstance(s);

    if (!is_ok) {
        /* Should detect OOM? */
        fprintf(stderr, "failed to compress data\n");
        return 0;
    } else if (ferror(fout)) {
        fprintf(stderr, "failed to write output\n");
        return 0;
    } else if (ferror(fin)) {
        fprintf(stderr, "failed to read input\n");
        return 0;
    }
    return 1;
}

int operation(const char *input_path, const char *output_path, int decompress, int q){
    char *dictionary_path = 0;
    int force = 1;
    int quality = 11;
    int repeat = 1;
    int verbose = 0;
    int lgwin = 22;
    int copy_stat = 1;
    clock_t clock_start;
    int i;
    if(q>0)
        quality = q;
    clock_start = clock();
    LOGD("quality:%d, lgwin:%d", quality, lgwin);
    for (i = 0; i < repeat; ++i) {
        FILE* fin = OpenInputFile(input_path);
        if(!fin){
            LOGE("Input file invalid, exit()");
            return 0;
        }
        FILE* fout = OpenOutputFile(output_path, force || (repeat > 1));
        if(!fout){
            LOGE("Output file invalid, exit()");
            return 0;
        }
        int is_ok = 0;
        if (decompress) {
          is_ok = Decompress(fin, fout, dictionary_path);
        } else {
          is_ok = Compress(quality, lgwin, fin, fout, dictionary_path);
        }
        if (!is_ok) {
          unlink(output_path);
        }
        if (fclose(fout) != 0) {
          LOGE("Close fout file error:%s", strerror(errno));
        }
        if (fclose(fin) != 0) {
          LOGE("Close fin file error:%s", strerror(errno));
        }
    }
    clock_t clock_end = clock();
    double duration = (double)(clock_end - clock_start) / CLOCKS_PER_SEC;
	if (duration < 1e-9) {
      duration = 1e-9;
    }
    LOGD("Duration:%f", duration);
    return 1;
}

JNIEXPORT jint JNICALL Java_com_fihtdc_libbrotlijni_BrotliUtils_compress
        (JNIEnv *env, jobject, jstring jinputPath, jstring joutputPath, jint jquality){
    const char *input_path = env->GetStringUTFChars(jinputPath, JNI_FALSE);
    const char *output_path = env->GetStringUTFChars(joutputPath, JNI_FALSE);
    LOGI("input_path:%s, output_path:%s, quality:%d", input_path, output_path, jquality);
    return operation(input_path, output_path, 0, (int)jquality);
}

JNIEXPORT jint JNICALL Java_com_fihtdc_libbrotlijni_BrotliUtils_decompress
        (JNIEnv *env, jobject, jstring jinputPath, jstring joutputPath){
    const char *input_path = env->GetStringUTFChars(jinputPath, JNI_FALSE);
    const char *output_path = env->GetStringUTFChars(joutputPath, JNI_FALSE);
    LOGI("input_path:%s, output_path:%s", input_path, output_path);
    return operation(input_path, output_path, 1, 0);
}