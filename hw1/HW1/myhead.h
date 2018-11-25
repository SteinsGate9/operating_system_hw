static void
err_doit(bool, int, const char *, va_list);

void
err_exit(int error, const char *fmt,...){
    va_list ap;
    va_start(ap,fmt);
    err_doit(true,error,fmt,ap);
    va_end(ap);
    exit(1);
}

static void
err_doit(bool errnoflag, int error, const char *fmt, va_list ap){
    char buf[MAXLINE];
    vsnprintf(buf,MAXLINE-1,fmt,ap);
    if(errnoflag)
        snprintf(buf+strlen(buf),MAXLINE-strlen(buf)-1,": %s",strerror(error));
    cerr<<buf<<endl;
}
