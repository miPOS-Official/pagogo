/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#pragma once
#include "pipe.cpp"

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class popen_t { 
protected:

    struct NODE {
        PROCESS_INFORMATION pi;
        file_t std_output;
        file_t std_input;
        file_t std_error;
        STARTUPINFO   si;
        int           fd;
        int     state =0;
    };  ptr_t<NODE> obj;

    template< class T >
    void _init_( const string_t& path, T& arg, T& env ) {

        SECURITY_ATTRIBUTES sa;
                            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                            sa.lpSecurityDescriptor = NULL;
                            sa.bInheritHandle = TRUE;

        HANDLE fda[2]; CreatePipe( &fda[0], &fda[1], &sa, CHUNK_SIZE, FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED );
        HANDLE fdb[2]; CreatePipe( &fdb[0], &fdb[1], &sa, CHUNK_SIZE, FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED );
        HANDLE fdc[2]; CreatePipe( &fdc[0], &fdc[1], &sa, CHUNK_SIZE, FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED );
        
        ZeroMemory(&obj->si, sizeof(STARTUPINFO));
        ZeroMemory(&obj->pi, sizeof(PROCESS_INFORMATION));
                    obj->si.cb         = sizeof( STARTUPINFO );
                    obj->si.hStdInput  = fda[0];
                    obj->si.hStdError  = fdc[1];
                    obj->si.hStdOutput = fdb[1];
                    obj->si.dwFlags   |= STARTF_USESTDHANDLES;
       
        auto cmd = arg.join(" "); auto ven = env.join("\0"); 
        auto dta = LPTSTR( ven.empty() ? NULL : ven.get() );

        obj->fd = ::CreateProcess( NULL, cmd.data(), NULL, NULL, 1, 0, dta, NULL, &obj->si, &obj->pi );
        WaitForSingleObject( obj->pi.hProcess, 0 ); WaitForSingleObject( obj->pi.hThread, 0 );

        if ( obj->fd != 0 ){ // Parent process
            obj->std_input  = { fda[1] };
            obj->std_output = { fdb[0] };
            obj->std_error  = { fdc[0] };
        } else {
            ::CloseHandle ( fda[0] ); ::CloseHandle ( fda[1] );
            ::CloseHandle ( fdb[0] ); ::CloseHandle ( fdb[1] );
            ::CloseHandle ( fdc[0] ); ::CloseHandle ( fdc[1] ); free();
        }

    }

public:

    event_t<>          onResume;
    event_t<except_t>  onError;
    event_t<>          onClose;
    event_t<>          onStop;
    event_t<>          onExit;
    event_t<>          onOpen;

    event_t<string_t>  onData;
    event_t<string_t>  onDout;
    event_t<string_t>  onDerr;

    virtual ~popen_t() noexcept {
        if( obj.count() > 1 ){ return; } 
        if( obj->state == 0 ){ return; } // free();
    }

    template< class... T >
    popen_t( const string_t& path, const initializer_t<string_t>& args ) : obj( new NODE() ) {
        array_t<const char*> arg; array_t<const char*> env; bool y=0;

        for ( auto x : args ) {
          if( x != nullptr && !y ) arg.push( x.get() );
        elif( x != nullptr &&  y ) env.push( x.get() );
        else  y =! y;
        }
        
        _init_( path, arg, env );
    }

    /*─······································································─*/

    bool is_alive() const noexcept { DWORD exitCode;
        if ( GetExitCodeProcess(obj->pi.hProcess,&exitCode) ) {
        if ( exitCode == STILL_ACTIVE ) { return true; }
        }    return false;
    }

    bool is_available() const noexcept { return is_closed() == false; }

    bool is_closed() const noexcept { return obj->state <= 0; }

    int get_fd() const noexcept { return obj->fd; }

    /*─······································································─*/

    virtual void free() const noexcept {
        if( obj->state == -3 && obj.count() > 1 ){ resume(); return; }
        if( obj->state == -2 ){ return; } obj->state = -2;
            obj->std_output.close(); obj->std_error .close(); 
            obj->std_input .close(); close(); kill(); 
            onClose.emit();
    }

    /*─······································································─*/
    
    void   kill() const noexcept { ::CloseHandle( obj->pi.hProcess ); ::CloseHandle( obj->pi.hThread ); }
    void  flush() const noexcept { std_input().flush(); std_output().flush(); std_error().flush(); }
    void resume() const noexcept { if(obj->state== 0) { return; } obj->state= 0; onResume.emit(); }
    void  close() const noexcept { if(obj->state < 0) { return; } obj->state=-1; onExit.emit(); }
    void   stop() const noexcept { if(obj->state==-3) { return; } obj->state=-3; onStop.emit(); }
    
    /*─······································································─*/

    void pipe() const noexcept { 
        if( obj->state == 1 ){ return; } obj->state = 1; onOpen.emit();
            ptr_t<_file_::read> _read1 = new _file_::read;
            ptr_t<_file_::read> _read2 = new _file_::read;
            auto self = type::bind( this );
            
        onExit([=](){ self->free(); });

        process::task::add([=](){
            if(!self->std_output().is_available() ){ self->close(); return -1; }
            if((*_read1)(&self->std_output())==1 ) { return  1; }
            if(  _read1->state <= 0 )              { return  1; }
            self->onData.emit(_read1->data);    
            self->onDout.emit(_read1->data);         return  1;
        });

        process::task::add([=](){
            if(!self->std_error().is_available() ){ self->close(); return -1; }
            if((*_read2)(&self->std_error())==1 ) { return  1; }
            if(  _read2->state <= 0 )             { return  1; }
            self->onData.emit(_read2->data);   
            self->onDerr.emit(_read2->data);        return  1;
        });

    }
    
    /*─······································································─*/

    template< class... T >
    int write( const T&... args )     const noexcept { return std_input().write( args... ); }

    template< class... T >
    string_t read( const T&... args ) const noexcept { return std_output().read( args... ); }
    
    /*─······································································─*/

    template< class... T >
    int _write( const T&... args ) const noexcept { return std_input()._write( args... ); }

    template< class... T >
    int _read( const T&... args )  const noexcept { return std_output()._read( args... ); }
    
    /*─······································································─*/

    file_t& std_error()  const noexcept { return obj->std_error;  }
    file_t& std_output() const noexcept { return obj->std_output; }
    file_t& std_input()  const noexcept { return obj->std_input;  }

};}

/*────────────────────────────────────────────────────────────────────────────*/