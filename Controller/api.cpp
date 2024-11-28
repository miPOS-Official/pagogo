#pragma once
#include <nodepp/https.h>

namespace page { namespace api {

    express_tcp_t add(){
        auto app = express::http::add();

        app.GET([]( express_http_t cli ){

            fetch_t args; ssl_t ssl;
                    args.method = "GET";
                    args.url    = "https://www.bcv.org.ve/";
                    args.headers= header_t({ 
                        { "Host", url::hostname(args.url) },
                        { "User-Agent", "Linux" } 
                    });

            https::fetch( args, &ssl ).then([=]( https_t ctx ){
                ptr_t<string_t> data = new string_t; stream::pipe( ctx );
                ctx.onData([=]( string_t chunk ){ *data += chunk; });
                ctx.onDrain([=](){
                    auto match = regex::match_all( *data, "<strong> \\d+,\\d+ </strong>" ).join();
                    auto value = regex::match_all( match, "\\d+,\\d+" );
                    for( auto x: value ){ cli.write( x + "<br>" ); }
                });
            }).fail([=](...){ cli.write("Something Went Wrong"); }); cli.send();

        });

        return app;
    }

}}