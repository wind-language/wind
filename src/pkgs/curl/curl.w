@include [
    "#libc.w"
    "curl.wi"
]

@linkflag("temp.o")
@extern func write_callback(contents: ptr, size: int64, nmemb: int64, userp: ptr): int64;

@pub func curl_test(): void {
    var curl: ptr;
    var res: int;
    var response:ptr = malloc(1);
    response[0] = 0;

    curl = curl_easy_init();
    printf("curl_easy_init() returned: %p\n", curl);
    branch [
        curl == 0: {
            free(response);
            return;
        }
    ]
    curl_easy_setopt(curl, 10002, "http://google.com");
    curl_easy_setopt(curl, 52, 1);
    curl_easy_setopt(curl, 20011, write_callback);
    curl_easy_setopt(curl, 10001, response);
    
    res = curl_easy_perform(curl);
    branch [
        res == 0: {
            printf("Response: %s\n", response);
            curl_easy_cleanup(curl);
        }
        else: {
            printf("curl_easy_perform() failed: %d\n", res);
        }
    ]
    free(response);
}