@include [
    "#libc.w"
    "curl.wi"
]

func write_cb(contents: ptr<char>, size: int64, nmemb: int64, userp: ptr<uint64>): int64 {
    // Register allocation is panicking in more complex indexing expressions
    var realsize: int64 = size * nmemb;
    var mem: ptr<ptr<char>> = userp;
    var alloc_ptr: ptr<char> = guard![ realloc(mem[0], mem[1] + realsize + 1) ];
    branch [
        alloc_ptr == Null: return 0;
    ]

    mem[0] = alloc_ptr;
    var new_addr: ptr<char> = mem[0];
    memcpy(
        new_addr + userp[1],
        contents,
        realsize
    );
    mem[1] = mem[1] + realsize;
    var mem_size: int64 = mem[1]-1;
    new_addr[mem_size] = 0;

    return realsize;
}

@pub func curl_test(): void {
    var url: [char; 64];
    printf("Enter URL: ");
    scanf("%64s", url);

    curl_global_init(CURL_GLOBAL_ALL);
    var curl: CURL = guard![curl_easy_init()];
    var res: CURLcode;

    var chunk: [uint64;2];
    chunk[0] = malloc(1);
    chunk[1] = 0;

    printf("chunk: %p\n", chunk);
    printf("chunk[0]: %p\n", chunk[0]);
    printf("chunk[1]: %llu\n", chunk[1]);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);

    res = curl_easy_perform(curl);

    branch [
        res == CURLE_OK: {
            printf("%llu bytes retrieved\n", chunk[1]);
            printf("%s\n", chunk[0]);
        }
    ]

    curl_easy_cleanup(curl);
    free(chunk[0]);
    curl_global_cleanup();
}