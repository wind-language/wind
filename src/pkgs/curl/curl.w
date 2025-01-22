@include [
    "#libc.wi"
    "curl.wi"
]

func write_cb(contents: ptr<char>, size: s64, nmemb: s64, userp: ptr<ptr<char>>): s64 {
    var realsize: s64 = size * nmemb;
    var alloc_ptr: ptr<char> = guard![ realloc(userp[0], (userp[1] + realsize + 1) as size_t) ];
    branch [
        alloc_ptr == Null: return 0;
    ]
    userp[0] = alloc_ptr;
    memcpy(
        userp[0] + userp[1],
        contents,
        realsize
    );
    userp[1] = userp[1] + realsize;
    var mem_size: s64 = userp[1]-1;
    var cast_ptr: ptr<char> = userp[0];
    cast_ptr[mem_size] = 0;

    return realsize;
}

namespace curl {
    @pub func test(): void {
        var url: [char; 64];
        printf("Enter URL: ");
        scanf("%64s", url);

        curl_global_init(CURL_GLOBAL_ALL);
        var curl: CURL = guard![curl_easy_init()];
        var res: CURLcode;

        var chunk: [ptr<char>;2];
        chunk[0] = malloc(1);
        chunk[1] = 1;

        printf("chunk: %p\n", chunk);
        printf("chunk[0]: %p\n", chunk[0]);
        printf("chunk[1]: %llu\n", chunk[1]);

        curl_easy_setopt(curl, CURLOPT_URL, url as u64);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb as u64);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk as u64);

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
}