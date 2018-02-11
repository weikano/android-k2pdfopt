# libk2pdfopt for Android

Library to view pdf/djvu files with text-reflow feature.

# Natives

Only recent (19+) phones has no issue loading natives. Old phones (16+) unable to locate library dependencies. Very old phones (15+) unable choice proper native library ABI version and crash with error UnsatifiedLinkExcetpion.

  * https://gitlab.com/axet/android-audio-library/blob/master/src/main/java/com/github/axet/audiolibrary/encoders/FormatMP3.java

```java
    import com.github.axet.androidlibrary.app.Natives;

    Native.loadLibraries(context, "k2pdfopt", "k2pdfoptjni")
```

  * https://gitlab.com/axet/android-library/blob/master/src/main/java/com/github/axet/androidlibrary/app/Natives.java


```gradle
dependencies {
    compile 'com.github.axet:k2pdfopt:0.0.3'
}
```

# Compile

    # gradle clean :libk2pdfopt:install install
