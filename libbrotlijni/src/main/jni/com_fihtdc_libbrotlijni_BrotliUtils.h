/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_fihtdc_libbrotlijni_BrotliUtils */

#ifndef _Included_com_fihtdc_libbrotlijni_BrotliUtils
#define _Included_com_fihtdc_libbrotlijni_BrotliUtils
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_fihtdc_libbrotlijni_BrotliUtils
 * Method:    compress
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_fihtdc_libbrotlijni_BrotliUtils_compress
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     com_fihtdc_libbrotlijni_BrotliUtils
 * Method:    decompress
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_fihtdc_libbrotlijni_BrotliUtils_decompress
  (JNIEnv *, jobject, jstring, jstring);

#ifdef __cplusplus
}
#endif
#endif
