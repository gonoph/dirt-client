#ifndef BUFFER_H
#define BUFFER_H

class Buffer {
public:
    Buffer (const Buffer&);
    Buffer(int min_size = 1024);

    void clear();                       // remove everything
    void shift (int count);             // shift count first bytes out of the buffer
    void unshift(const char*, int);     // put that many bytes at the beginning of the buffer
    int count() const { return len; }   // number of bytes *in use*

    char * get(int size); // Get that large a buffer at the end
    void use (int size);  // Just used that many bytes of the get'ed buffer
    
    operator const char *() const { return data; };
    const char *operator~() const { return data; }
    virtual ~Buffer()                           { delete[] (data);}

    // Flush: see if we can get the current output out so there is more space
    virtual bool flush() { return false; }

    bool strcat(const char*);               // append text
    bool strncat (const char *s, int len);  // append binary data
    int printf (const char *fmt, ...) __attribute__((format(printf,2,3)));

    bool chrcat(int c) {                    // quick append of one character
        if (len < size-2) { // quick
            data[len++] = c;
            data[len] = 0;
            return true;
        } else {
            char buf[2] = { c, 0 };
            return strcat(buf);
        }
    }
    
protected:
    
    int find_mem_size(int min_size);
    void need(int);
    
    int size;       /* Allocated memory size */
    char *data;
    int len;        /* Currently used length */
    bool overflowed;
    
};
#endif
