char *strcat(char *dst, const char *src){
    char *end = dst;
    while (*end != 0) {
        ++end;
    }
    while (*src) {
        *end = *src;
        end++;
        src++;
    }
    *end = 0;
    return dst;
}
