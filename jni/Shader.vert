static const char* VERTEX_SHADER =    
        "attribute vec4 aPosition;\n" 
        "attribute vec2 aTextureCoord;\n" 
        "varying vec2 tc;\n" 
        "void main() {\n" 
        "  gl_Position = aPosition;\n" 
        "  tc = aTextureCoord;\n" 
        "}\n";