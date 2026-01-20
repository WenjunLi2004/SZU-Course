#include "Angel.h"

namespace Angel {

static char*
readShaderSource(const char* shaderFile)
{
#ifdef __APPLE__
    FILE *fp;
    fp = fopen(shaderFile, "r");
#else
    FILE *fp;
    fopen_s(&fp, shaderFile, "r");
#endif

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];

    memset(buf, 0, size + 1);

    fread(buf, 1, size, fp);

    buf[size] = '\0';
    fclose(fp);

    return buf;
}

GLuint InitShader( const char* vertexShaderFile, const char* fragmentShaderFile )
{
    struct Shader { const char* filename; GLenum type; GLchar* source; } shaders[2] = {
        { vertexShaderFile, GL_VERTEX_SHADER, NULL },
        { fragmentShaderFile, GL_FRAGMENT_SHADER, NULL }
    };

    GLuint program = glCreateProgram();

    for ( int i = 0; i < 2; ++i ) {
        Shader& s = shaders[i];
        s.source = readShaderSource( s.filename );
        if ( s.source == NULL ) {
            std::cerr << "Unable to load shader '" << s.filename << "'" << std::endl;
            exit( EXIT_FAILURE );
        }
        GLuint shader = glCreateShader( s.type );
        glShaderSource( shader, 1, (const GLchar**) &s.source, NULL );
        glCompileShader( shader );

        GLint compiled;
        glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
        if ( !compiled ) {
            GLint logSize;
            glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logSize );
            char* logMsg = new char[logSize];
            glGetShaderInfoLog( shader, logSize, NULL, logMsg );
            std::cerr << logMsg << std::endl;
            delete [] logMsg;
            exit( EXIT_FAILURE );
        }
        glAttachShader( program, shader );
    }

    glLinkProgram( program );

    GLint linked;
    glGetProgramiv( program, GL_LINK_STATUS, &linked );
    if ( !linked ) {
        GLint logSize;
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logSize);
        char* logMsg = new char[logSize];
        glGetProgramInfoLog( program, logSize, NULL, logMsg );
        std::cerr << logMsg << std::endl;
        delete [] logMsg;
        exit( EXIT_FAILURE );
    }

    glUseProgram(program);
    return program;
}

}  // namespace Angel