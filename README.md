# BrotliJNI
JNI version Brotli compression tool, implement with Google Brotli project

Brotli compression source code from [Here](https://github.com/google/brotli)

### Modules:

1. libbrotlijni: Java library for use JNI code to do compress/decompress
2. brotlitest: Sample app to include library and compress/decompress files

### Setup

1. Build Jar file (using gradle script)

    Gradle -> libbrotlijni -> Tasks -> other -> makeJar

2. Build .so files (using ndk)

    2.1 Setup external tool in Android Studio
    
    Settings -> Tools -> External tools -> [Plus button] -> Fill values
        
    ![ndkbuild](https://cloud.githubusercontent.com/assets/3983418/24185833/3357afa4-0f10-11e7-8d3d-cb79c611fc27.png)
        
    Configs
    
    Program: [path of ndk-build.cmd in NDK]
    
    Parameters: 
       
       NDK_PROJECT_PATH=$ModuleFileDir$/build/intermediates/ndk NDK_LIBS_OUT=$ModuleFileDir$/src/main/jniLibs NDK_APPLICATION_MK=$ModuleFileDir$/src/main/jni/Application.mk APP_BUILD_SCRIPT=$ModuleFileDir$/src/main/jni/Android.mk V=1
       
    Working directory: $ProjectFileDir$
    
    2.2 Build so files: [Right click on libbrotlijni module] -> External Tools -> NDKBuild
    
3. Include in your project

    Put libBtotliJNI.jar into [Module]/libs
    
    Put jniLibs folder into [Module]/src/main
    
    Add content into [Module]/build.gradle
    
        sourceSets.main{
            jni.srcDirs=[]
            jniLibs.srcDir "src/main/jniLibs"
        }
