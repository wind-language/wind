@include [
    "#libc.w"
    "curl.wi"
]

func mem_cb(contents: ptr<char>, size: int64, nmemb: int64, userp: ptr<uint64>): uint64 {
    var realsize: uint64 = size * nmemb;
    userp[0] = realloc(userp[0], userp[1] + realsize + 1);
    guard![userp[0]];


    var usum: ptr<char> = userp[0] + userp[1];
    memcpy(usum, contents, realsize);
    userp[1] = userp[1] + realsize;
    usum[0] = 0;

    return realsize;
}

@pub func curl_test(): void {
    curl_global_init(CURL_GLOBAL_ALL);
    var curl: CURL = guard![curl_easy_init()];
    var res: CURLcode;

    // We don't have structs :(
    var chunk: ptr<uint64> = malloc(16);
    chunk[0] = malloc(1);
    chunk[1] = 0;

    printf("chunk: %p\n", chunk);
    printf("chunk[0]: %p\n", chunk[0]);
    printf("chunk[1]: %llu\n", chunk[1]);

    curl_easy_setopt(curl, CURLOPT_URL, "http://captive.apple.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);

    branch [
        res == CURLE_OK: {
            printf("%llu bytes retrieved\n", chunk[1]);
        }
    ]


    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    free(chunk[0]);
    curl_global_cleanup();
}