#ifndef STATICBUFFER_H
#define STATICBUFFER_H

class StaticBuffer
{
public:
    StaticBuffer(int sz = 16384);
    StaticBuffer(const char *);

    ~StaticBuffer();

    int size() const {
        return len;
    }

    operator char *() { return s; }
    static char* sprintf(const char *fmt, ...);

private:
    char *s;
    int len;
};
const char* Sprintf(const char *fmt, ...) __attribute__((format(printf,1,2)));

// Hmm, dunno about this one
// typedef class StaticBuffer SBuf;

#endif
