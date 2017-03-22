package com.fihtdc.brotlitest;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.fihtdc.libbrotlijni.BrotliUtils;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "BrotliTest/MainActivity";
    BrotliUtils brotli;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        brotli = new BrotliUtils();
        Button btn1 = (Button) this.findViewById(R.id.button1);
        btn1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                compressFile(Environment.getExternalStorageDirectory()+"/Google.html");
            }
        });
        Button btn2 = (Button) this.findViewById(R.id.button2);
        btn2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                decompressFile(Environment.getExternalStorageDirectory()+"/Google.html.bro");
            }
        });
    }

    private void compressFile(String input) {
        File inputFile = new File(input);
        String path = inputFile.getParent();
        File outputFile = new File(path, inputFile.getName() + ".bro");
        String output = outputFile.getPath();
        Log.d(TAG, "input:" + input + ", output:" + output);
        brotli.compress(input, output, 0);
    }

    private void decompressFile(String input) {
        File inputFile = new File(input);
        String path = inputFile.getParent();
        File outputFile = new File(path, inputFile.getName() + ".origin");
        String output = outputFile.getPath();
        Log.d(TAG, "input:" + input + ", output:" + output);
        brotli.decompress(input, output);
    }

}
