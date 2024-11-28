#define MAX_FILENO 1048576

#include <nodepp/nodepp.h>
#include <express/http.h>
#include <nodepp/query.h>
#include <nodepp/fs.h>

using namespace nodepp;

//#include <nodepp/cluster.h>
#include "./Controller/api.cpp"

void onMain() {
    
    auto app = express::http::add();
    
    app.USE( "/api", page::api::add() );
    app.USE( express::http::ssr( "View" ) );

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at http://localhost:8000" );
    });

}