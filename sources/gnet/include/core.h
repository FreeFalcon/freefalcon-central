
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Sun Aug 20 16:02:20 2000
 */
/* Compiler settings for E:\coding\projects\GNet\Shared\Core.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __Core_h__
#define __Core_h__

/* Forward Declarations */ 

#ifndef __IWorkerThread_FWD_DEFINED__
#define __IWorkerThread_FWD_DEFINED__
typedef interface IWorkerThread IWorkerThread;
#endif 	/* __IWorkerThread_FWD_DEFINED__ */


#ifndef __IBeaconListener_FWD_DEFINED__
#define __IBeaconListener_FWD_DEFINED__
typedef interface IBeaconListener IBeaconListener;
#endif 	/* __IBeaconListener_FWD_DEFINED__ */


#ifndef __IGame_FWD_DEFINED__
#define __IGame_FWD_DEFINED__
typedef interface IGame IGame;
#endif 	/* __IGame_FWD_DEFINED__ */


#ifndef __IGameEvents_FWD_DEFINED__
#define __IGameEvents_FWD_DEFINED__
typedef interface IGameEvents IGameEvents;
#endif 	/* __IGameEvents_FWD_DEFINED__ */


#ifndef __IPlayer_FWD_DEFINED__
#define __IPlayer_FWD_DEFINED__
typedef interface IPlayer IPlayer;
#endif 	/* __IPlayer_FWD_DEFINED__ */


#ifndef __IMasterServer_FWD_DEFINED__
#define __IMasterServer_FWD_DEFINED__
typedef interface IMasterServer IMasterServer;
#endif 	/* __IMasterServer_FWD_DEFINED__ */


#ifndef __IRemoteMasterServer_FWD_DEFINED__
#define __IRemoteMasterServer_FWD_DEFINED__
typedef interface IRemoteMasterServer IRemoteMasterServer;
#endif 	/* __IRemoteMasterServer_FWD_DEFINED__ */


#ifndef __IRemoteMasterServerEvents_FWD_DEFINED__
#define __IRemoteMasterServerEvents_FWD_DEFINED__
typedef interface IRemoteMasterServerEvents IRemoteMasterServerEvents;
#endif 	/* __IRemoteMasterServerEvents_FWD_DEFINED__ */


#ifndef __IMasterServerAdministration_FWD_DEFINED__
#define __IMasterServerAdministration_FWD_DEFINED__
typedef interface IMasterServerAdministration IMasterServerAdministration;
#endif 	/* __IMasterServerAdministration_FWD_DEFINED__ */


#ifndef __IMasterServerEvents_FWD_DEFINED__
#define __IMasterServerEvents_FWD_DEFINED__
typedef interface IMasterServerEvents IMasterServerEvents;
#endif 	/* __IMasterServerEvents_FWD_DEFINED__ */


#ifndef __IHost_FWD_DEFINED__
#define __IHost_FWD_DEFINED__
typedef interface IHost IHost;
#endif 	/* __IHost_FWD_DEFINED__ */


#ifndef __IHostInfo_FWD_DEFINED__
#define __IHostInfo_FWD_DEFINED__
typedef interface IHostInfo IHostInfo;
#endif 	/* __IHostInfo_FWD_DEFINED__ */


#ifndef __IBeaconListenerEvents_FWD_DEFINED__
#define __IBeaconListenerEvents_FWD_DEFINED__
typedef interface IBeaconListenerEvents IBeaconListenerEvents;
#endif 	/* __IBeaconListenerEvents_FWD_DEFINED__ */


#ifndef __IUplink_FWD_DEFINED__
#define __IUplink_FWD_DEFINED__
typedef interface IUplink IUplink;
#endif 	/* __IUplink_FWD_DEFINED__ */


#ifndef __IProvideGUID_FWD_DEFINED__
#define __IProvideGUID_FWD_DEFINED__
typedef interface IProvideGUID IProvideGUID;
#endif 	/* __IProvideGUID_FWD_DEFINED__ */


#ifndef __IListViewItem_FWD_DEFINED__
#define __IListViewItem_FWD_DEFINED__
typedef interface IListViewItem IListViewItem;
#endif 	/* __IListViewItem_FWD_DEFINED__ */


#ifndef __IListViewItemContainer_FWD_DEFINED__
#define __IListViewItemContainer_FWD_DEFINED__
typedef interface IListViewItemContainer IListViewItemContainer;
#endif 	/* __IListViewItemContainer_FWD_DEFINED__ */


#ifndef __IListView_FWD_DEFINED__
#define __IListView_FWD_DEFINED__
typedef interface IListView IListView;
#endif 	/* __IListView_FWD_DEFINED__ */


#ifndef __IObjectManager_FWD_DEFINED__
#define __IObjectManager_FWD_DEFINED__
typedef interface IObjectManager IObjectManager;
#endif 	/* __IObjectManager_FWD_DEFINED__ */


#ifndef __IServerObjectManager_FWD_DEFINED__
#define __IServerObjectManager_FWD_DEFINED__
typedef interface IServerObjectManager IServerObjectManager;
#endif 	/* __IServerObjectManager_FWD_DEFINED__ */


#ifndef __IWorkerThread_FWD_DEFINED__
#define __IWorkerThread_FWD_DEFINED__
typedef interface IWorkerThread IWorkerThread;
#endif 	/* __IWorkerThread_FWD_DEFINED__ */


#ifndef __IBeaconListener_FWD_DEFINED__
#define __IBeaconListener_FWD_DEFINED__
typedef interface IBeaconListener IBeaconListener;
#endif 	/* __IBeaconListener_FWD_DEFINED__ */


#ifndef __IGame_FWD_DEFINED__
#define __IGame_FWD_DEFINED__
typedef interface IGame IGame;
#endif 	/* __IGame_FWD_DEFINED__ */


#ifndef __IGameEvents_FWD_DEFINED__
#define __IGameEvents_FWD_DEFINED__
typedef interface IGameEvents IGameEvents;
#endif 	/* __IGameEvents_FWD_DEFINED__ */


#ifndef __IPlayer_FWD_DEFINED__
#define __IPlayer_FWD_DEFINED__
typedef interface IPlayer IPlayer;
#endif 	/* __IPlayer_FWD_DEFINED__ */


#ifndef __IMasterServer_FWD_DEFINED__
#define __IMasterServer_FWD_DEFINED__
typedef interface IMasterServer IMasterServer;
#endif 	/* __IMasterServer_FWD_DEFINED__ */


#ifndef __IMasterServerAdministration_FWD_DEFINED__
#define __IMasterServerAdministration_FWD_DEFINED__
typedef interface IMasterServerAdministration IMasterServerAdministration;
#endif 	/* __IMasterServerAdministration_FWD_DEFINED__ */


#ifndef __IMasterServerEvents_FWD_DEFINED__
#define __IMasterServerEvents_FWD_DEFINED__
typedef interface IMasterServerEvents IMasterServerEvents;
#endif 	/* __IMasterServerEvents_FWD_DEFINED__ */


#ifndef __IRemoteMasterServer_FWD_DEFINED__
#define __IRemoteMasterServer_FWD_DEFINED__
typedef interface IRemoteMasterServer IRemoteMasterServer;
#endif 	/* __IRemoteMasterServer_FWD_DEFINED__ */


#ifndef __IRemoteMasterServerEvents_FWD_DEFINED__
#define __IRemoteMasterServerEvents_FWD_DEFINED__
typedef interface IRemoteMasterServerEvents IRemoteMasterServerEvents;
#endif 	/* __IRemoteMasterServerEvents_FWD_DEFINED__ */


#ifndef __IHost_FWD_DEFINED__
#define __IHost_FWD_DEFINED__
typedef interface IHost IHost;
#endif 	/* __IHost_FWD_DEFINED__ */


#ifndef __IBeaconListenerEvents_FWD_DEFINED__
#define __IBeaconListenerEvents_FWD_DEFINED__
typedef interface IBeaconListenerEvents IBeaconListenerEvents;
#endif 	/* __IBeaconListenerEvents_FWD_DEFINED__ */


#ifndef __IProvideGUID_FWD_DEFINED__
#define __IProvideGUID_FWD_DEFINED__
typedef interface IProvideGUID IProvideGUID;
#endif 	/* __IProvideGUID_FWD_DEFINED__ */


#ifndef __IListViewItem_FWD_DEFINED__
#define __IListViewItem_FWD_DEFINED__
typedef interface IListViewItem IListViewItem;
#endif 	/* __IListViewItem_FWD_DEFINED__ */


#ifndef __IListViewItemContainer_FWD_DEFINED__
#define __IListViewItemContainer_FWD_DEFINED__
typedef interface IListViewItemContainer IListViewItemContainer;
#endif 	/* __IListViewItemContainer_FWD_DEFINED__ */


#ifndef __IListView_FWD_DEFINED__
#define __IListView_FWD_DEFINED__
typedef interface IListView IListView;
#endif 	/* __IListView_FWD_DEFINED__ */


#ifndef __IObjectManager_FWD_DEFINED__
#define __IObjectManager_FWD_DEFINED__
typedef interface IObjectManager IObjectManager;
#endif 	/* __IObjectManager_FWD_DEFINED__ */


#ifndef __IServerObjectManager_FWD_DEFINED__
#define __IServerObjectManager_FWD_DEFINED__
typedef interface IServerObjectManager IServerObjectManager;
#endif 	/* __IServerObjectManager_FWD_DEFINED__ */


#ifndef __IUplink_FWD_DEFINED__
#define __IUplink_FWD_DEFINED__
typedef interface IUplink IUplink;
#endif 	/* __IUplink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IWorkerThread_INTERFACE_DEFINED__
#define __IWorkerThread_INTERFACE_DEFINED__

/* interface IWorkerThread */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWorkerThread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8EDBEF0A-AEB1-48ad-9ED2-BD41CD1075BD")
    IWorkerThread : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WorkerExecute( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWorkerThreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWorkerThread __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWorkerThread __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWorkerThread __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WorkerExecute )( 
            IWorkerThread __RPC_FAR * This);
        
        END_INTERFACE
    } IWorkerThreadVtbl;

    interface IWorkerThread
    {
        CONST_VTBL struct IWorkerThreadVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWorkerThread_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWorkerThread_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWorkerThread_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWorkerThread_WorkerExecute(This)	\
    (This)->lpVtbl -> WorkerExecute(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWorkerThread_WorkerExecute_Proxy( 
    IWorkerThread __RPC_FAR * This);


void __RPC_STUB IWorkerThread_WorkerExecute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWorkerThread_INTERFACE_DEFINED__ */


#ifndef __IBeaconListener_INTERFACE_DEFINED__
#define __IBeaconListener_INTERFACE_DEFINED__

/* interface IBeaconListener */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBeaconListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57120-D4E3-11D2-9018-004F4E006398")
    IBeaconListener : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBeaconListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBeaconListener __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBeaconListener __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBeaconListener __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IBeaconListener __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IBeaconListener __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IBeaconListener __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IBeaconListener __RPC_FAR * This);
        
        END_INTERFACE
    } IBeaconListenerVtbl;

    interface IBeaconListener
    {
        CONST_VTBL struct IBeaconListenerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBeaconListener_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBeaconListener_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBeaconListener_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBeaconListener_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IBeaconListener_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IBeaconListener_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IBeaconListener_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IBeaconListener_get_Name_Proxy( 
    IBeaconListener __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IBeaconListener_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IBeaconListener_put_Name_Proxy( 
    IBeaconListener __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IBeaconListener_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBeaconListener_Start_Proxy( 
    IBeaconListener __RPC_FAR * This);


void __RPC_STUB IBeaconListener_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBeaconListener_Stop_Proxy( 
    IBeaconListener __RPC_FAR * This);


void __RPC_STUB IBeaconListener_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBeaconListener_INTERFACE_DEFINED__ */


#ifndef __IGame_INTERFACE_DEFINED__
#define __IGame_INTERFACE_DEFINED__

/* interface IGame */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57123-D4E3-11D2-9018-004F4E006398")
    IGame : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Players( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_NumPlayers( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MaxPlayers( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Mode( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGame __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGame __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGame __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Players )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IGame __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumPlayers )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxPlayers )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerName )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerName )( 
            IGame __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IGame __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Mode )( 
            IGame __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IGameVtbl;

    interface IGame
    {
        CONST_VTBL struct IGameVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGame_get_Players(This,pVal)	\
    (This)->lpVtbl -> get_Players(This,pVal)

#define IGame_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IGame_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IGame_get_NumPlayers(This,pVal)	\
    (This)->lpVtbl -> get_NumPlayers(This,pVal)

#define IGame_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IGame_get_MaxPlayers(This,pVal)	\
    (This)->lpVtbl -> get_MaxPlayers(This,pVal)

#define IGame_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define IGame_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IGame_get_ServerName(This,pVal)	\
    (This)->lpVtbl -> get_ServerName(This,pVal)

#define IGame_put_ServerName(This,newVal)	\
    (This)->lpVtbl -> put_ServerName(This,newVal)

#define IGame_Update(This)	\
    (This)->lpVtbl -> Update(This)

#define IGame_get_Mode(This,pVal)	\
    (This)->lpVtbl -> get_Mode(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Players_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IGame_get_Players_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Name_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IGame_put_Name_Proxy( 
    IGame __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IGame_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_NumPlayers_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IGame_get_NumPlayers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Type_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_MaxPlayers_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IGame_get_MaxPlayers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Version_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Location_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_ServerName_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IGame_put_ServerName_Proxy( 
    IGame __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IGame_put_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGame_Update_Proxy( 
    IGame __RPC_FAR * This);


void __RPC_STUB IGame_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGame_get_Mode_Proxy( 
    IGame __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IGame_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGame_INTERFACE_DEFINED__ */


#ifndef __IGameEvents_INTERFACE_DEFINED__
#define __IGameEvents_INTERFACE_DEFINED__

/* interface IGameEvents */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGameEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B8D1F614-D5E4-4bd1-8F01-17371DEAF685")
    IGameEvents : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateComplete( 
            IGame __RPC_FAR *Game) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateAborted( 
            IGame __RPC_FAR *Game) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGameEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGameEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGameEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGameEvents __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateComplete )( 
            IGameEvents __RPC_FAR * This,
            IGame __RPC_FAR *Game);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateAborted )( 
            IGameEvents __RPC_FAR * This,
            IGame __RPC_FAR *Game);
        
        END_INTERFACE
    } IGameEventsVtbl;

    interface IGameEvents
    {
        CONST_VTBL struct IGameEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGameEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGameEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGameEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGameEvents_UpdateComplete(This,Game)	\
    (This)->lpVtbl -> UpdateComplete(This,Game)

#define IGameEvents_UpdateAborted(This,Game)	\
    (This)->lpVtbl -> UpdateAborted(This,Game)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGameEvents_UpdateComplete_Proxy( 
    IGameEvents __RPC_FAR * This,
    IGame __RPC_FAR *Game);


void __RPC_STUB IGameEvents_UpdateComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGameEvents_UpdateAborted_Proxy( 
    IGameEvents __RPC_FAR * This,
    IGame __RPC_FAR *Game);


void __RPC_STUB IGameEvents_UpdateAborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGameEvents_INTERFACE_DEFINED__ */


#ifndef __IPlayer_INTERFACE_DEFINED__
#define __IPlayer_INTERFACE_DEFINED__

/* interface IPlayer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IPlayer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57125-D4E3-11D2-9018-004F4E006398")
    IPlayer : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPlayerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPlayer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPlayer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPlayer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IPlayer __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IPlayerVtbl;

    interface IPlayer
    {
        CONST_VTBL struct IPlayerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlayer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPlayer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPlayer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPlayer_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IPlayer_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IPlayer_get_Name_Proxy( 
    IPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IPlayer_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IPlayer_put_Name_Proxy( 
    IPlayer __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPlayer_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPlayer_INTERFACE_DEFINED__ */


#ifndef __IMasterServer_INTERFACE_DEFINED__
#define __IMasterServer_INTERFACE_DEFINED__

/* interface IMasterServer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMasterServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57127-D4E3-11D2-9018-004F4E006398")
    IMasterServer : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AdminEmail( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MOTD( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HostInfos( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMasterServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMasterServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMasterServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMasterServer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AdminEmail )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MOTD )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HostInfos )( 
            IMasterServer __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        END_INTERFACE
    } IMasterServerVtbl;

    interface IMasterServer
    {
        CONST_VTBL struct IMasterServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMasterServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMasterServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMasterServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMasterServer_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IMasterServer_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IMasterServer_get_AdminEmail(This,pVal)	\
    (This)->lpVtbl -> get_AdminEmail(This,pVal)

#define IMasterServer_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define IMasterServer_get_MOTD(This,pVal)	\
    (This)->lpVtbl -> get_MOTD(This,pVal)

#define IMasterServer_get_HostInfos(This,pVal)	\
    (This)->lpVtbl -> get_HostInfos(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_Name_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_Location_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_AdminEmail_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_AdminEmail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_Version_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_MOTD_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_MOTD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServer_get_HostInfos_Proxy( 
    IMasterServer __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IMasterServer_get_HostInfos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMasterServer_INTERFACE_DEFINED__ */


#ifndef __IRemoteMasterServer_INTERFACE_DEFINED__
#define __IRemoteMasterServer_INTERFACE_DEFINED__

/* interface IRemoteMasterServer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRemoteMasterServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E0016343-4124-48a9-9734-93F48700A04A")
    IRemoteMasterServer : public IMasterServer
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Servername( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Servername( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Port( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Port( 
            long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MaxConcurrentHostQueries( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MaxConcurrentHostQueries( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CancelUpdate( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GameFilter( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_GameFilter( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteMasterServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteMasterServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteMasterServer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AdminEmail )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MOTD )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HostInfos )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Servername )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Servername )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Port )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Port )( 
            IRemoteMasterServer __RPC_FAR * This,
            long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxConcurrentHostQueries )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxConcurrentHostQueries )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IRemoteMasterServer __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelUpdate )( 
            IRemoteMasterServer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GameFilter )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GameFilter )( 
            IRemoteMasterServer __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IRemoteMasterServerVtbl;

    interface IRemoteMasterServer
    {
        CONST_VTBL struct IRemoteMasterServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteMasterServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteMasterServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteMasterServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteMasterServer_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IRemoteMasterServer_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IRemoteMasterServer_get_AdminEmail(This,pVal)	\
    (This)->lpVtbl -> get_AdminEmail(This,pVal)

#define IRemoteMasterServer_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define IRemoteMasterServer_get_MOTD(This,pVal)	\
    (This)->lpVtbl -> get_MOTD(This,pVal)

#define IRemoteMasterServer_get_HostInfos(This,pVal)	\
    (This)->lpVtbl -> get_HostInfos(This,pVal)


#define IRemoteMasterServer_get_Servername(This,pVal)	\
    (This)->lpVtbl -> get_Servername(This,pVal)

#define IRemoteMasterServer_put_Servername(This,newVal)	\
    (This)->lpVtbl -> put_Servername(This,newVal)

#define IRemoteMasterServer_get_Port(This,pVal)	\
    (This)->lpVtbl -> get_Port(This,pVal)

#define IRemoteMasterServer_put_Port(This,newVal)	\
    (This)->lpVtbl -> put_Port(This,newVal)

#define IRemoteMasterServer_get_MaxConcurrentHostQueries(This,pVal)	\
    (This)->lpVtbl -> get_MaxConcurrentHostQueries(This,pVal)

#define IRemoteMasterServer_put_MaxConcurrentHostQueries(This,newVal)	\
    (This)->lpVtbl -> put_MaxConcurrentHostQueries(This,newVal)

#define IRemoteMasterServer_Update(This)	\
    (This)->lpVtbl -> Update(This)

#define IRemoteMasterServer_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)

#define IRemoteMasterServer_get_GameFilter(This,pVal)	\
    (This)->lpVtbl -> get_GameFilter(This,pVal)

#define IRemoteMasterServer_put_GameFilter(This,newVal)	\
    (This)->lpVtbl -> put_GameFilter(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_get_Servername_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRemoteMasterServer_get_Servername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_put_Servername_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRemoteMasterServer_put_Servername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_get_Port_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRemoteMasterServer_get_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_put_Port_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    long newVal);


void __RPC_STUB IRemoteMasterServer_put_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_get_MaxConcurrentHostQueries_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRemoteMasterServer_get_MaxConcurrentHostQueries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_put_MaxConcurrentHostQueries_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRemoteMasterServer_put_MaxConcurrentHostQueries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_Update_Proxy( 
    IRemoteMasterServer __RPC_FAR * This);


void __RPC_STUB IRemoteMasterServer_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_CancelUpdate_Proxy( 
    IRemoteMasterServer __RPC_FAR * This);


void __RPC_STUB IRemoteMasterServer_CancelUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_get_GameFilter_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRemoteMasterServer_get_GameFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServer_put_GameFilter_Proxy( 
    IRemoteMasterServer __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRemoteMasterServer_put_GameFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteMasterServer_INTERFACE_DEFINED__ */


#ifndef __IRemoteMasterServerEvents_INTERFACE_DEFINED__
#define __IRemoteMasterServerEvents_INTERFACE_DEFINED__

/* interface IRemoteMasterServerEvents */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRemoteMasterServerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("36B5B50D-69BB-43d5-83F6-AAF7EB9FFF7B")
    IRemoteMasterServerEvents : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ServerListReceived( 
            int nServers) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateComplete( 
            BOOL bSuccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteMasterServerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteMasterServerEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteMasterServerEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteMasterServerEvents __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ServerListReceived )( 
            IRemoteMasterServerEvents __RPC_FAR * This,
            int nServers);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateComplete )( 
            IRemoteMasterServerEvents __RPC_FAR * This,
            BOOL bSuccess);
        
        END_INTERFACE
    } IRemoteMasterServerEventsVtbl;

    interface IRemoteMasterServerEvents
    {
        CONST_VTBL struct IRemoteMasterServerEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteMasterServerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteMasterServerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteMasterServerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteMasterServerEvents_ServerListReceived(This,nServers)	\
    (This)->lpVtbl -> ServerListReceived(This,nServers)

#define IRemoteMasterServerEvents_UpdateComplete(This,bSuccess)	\
    (This)->lpVtbl -> UpdateComplete(This,bSuccess)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServerEvents_ServerListReceived_Proxy( 
    IRemoteMasterServerEvents __RPC_FAR * This,
    int nServers);


void __RPC_STUB IRemoteMasterServerEvents_ServerListReceived_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMasterServerEvents_UpdateComplete_Proxy( 
    IRemoteMasterServerEvents __RPC_FAR * This,
    BOOL bSuccess);


void __RPC_STUB IRemoteMasterServerEvents_UpdateComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteMasterServerEvents_INTERFACE_DEFINED__ */


#ifndef __IMasterServerAdministration_INTERFACE_DEFINED__
#define __IMasterServerAdministration_INTERFACE_DEFINED__

/* interface IMasterServerAdministration */
/* [unique][helpstring][uuid][object] */ 

typedef /* [public][public][public][public] */ struct __MIDL_IMasterServerAdministration_0001
    {
    long timeStarted;
    DWORD nNumberOfRecords;
    DWORD dwAverageQueriesPerSecond;
    DWORD dwLastQueriesPerSecond;
    DWORD dwMinQueriesPerSecond;
    DWORD dwMaxQueriesPerSecond;
    __int64 nTotalQueries;
    DWORD dwAverageTransactionsPerSecond;
    DWORD dwLastTransactionsPerSecond;
    DWORD dwMinTransactionsPerSecond;
    DWORD dwMaxTransactionsPerSecond;
    __int64 nTotalTransactions;
    }	IMasterServerAdministration_Statistics;

typedef /* [public][public] */ struct __MIDL_IMasterServerAdministration_0002
    {
    BSTR m_bstrName;
    BSTR m_bstrLocation;
    BSTR m_bstrAdminEmail;
    BSTR m_bstrVersion;
    BSTR m_bstrMOTD;
    BSTR m_bstrLogPath;
    byte m_nLogLevel;
    IMasterServerAdministration_Statistics m_Stats;
    }	IMasterServerAdministration_PropertySet;


EXTERN_C const IID IID_IMasterServerAdministration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9D9BB9A8-D54C-11d2-9018-004F4E006398")
    IMasterServerAdministration : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BeaconListeners( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Location( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AdminEmail( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Statistics( 
            /* [retval][out] */ IMasterServerAdministration_Statistics __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ IMasterServerAdministration_PropertySet __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MOTD( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IsStarted( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LogLevel( 
            /* [retval][out] */ byte __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LogLevel( 
            byte newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMasterServerAdministrationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMasterServerAdministration __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMasterServerAdministration __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IMasterServerAdministration __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IMasterServerAdministration __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BeaconListeners )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Location )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AdminEmail )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Statistics )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [retval][out] */ IMasterServerAdministration_Statistics __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [retval][out] */ IMasterServerAdministration_PropertySet __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MOTD )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsStarted )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LogLevel )( 
            IMasterServerAdministration __RPC_FAR * This,
            /* [retval][out] */ byte __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LogLevel )( 
            IMasterServerAdministration __RPC_FAR * This,
            byte newVal);
        
        END_INTERFACE
    } IMasterServerAdministrationVtbl;

    interface IMasterServerAdministration
    {
        CONST_VTBL struct IMasterServerAdministrationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMasterServerAdministration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMasterServerAdministration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMasterServerAdministration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMasterServerAdministration_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IMasterServerAdministration_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IMasterServerAdministration_get_BeaconListeners(This,pVal)	\
    (This)->lpVtbl -> get_BeaconListeners(This,pVal)

#define IMasterServerAdministration_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IMasterServerAdministration_put_Location(This,newVal)	\
    (This)->lpVtbl -> put_Location(This,newVal)

#define IMasterServerAdministration_put_AdminEmail(This,newVal)	\
    (This)->lpVtbl -> put_AdminEmail(This,newVal)

#define IMasterServerAdministration_get_Statistics(This,pVal)	\
    (This)->lpVtbl -> get_Statistics(This,pVal)

#define IMasterServerAdministration_get_Properties(This,pVal)	\
    (This)->lpVtbl -> get_Properties(This,pVal)

#define IMasterServerAdministration_put_MOTD(This,newVal)	\
    (This)->lpVtbl -> put_MOTD(This,newVal)

#define IMasterServerAdministration_get_IsStarted(This,pVal)	\
    (This)->lpVtbl -> get_IsStarted(This,pVal)

#define IMasterServerAdministration_get_LogLevel(This,pVal)	\
    (This)->lpVtbl -> get_LogLevel(This,pVal)

#define IMasterServerAdministration_put_LogLevel(This,newVal)	\
    (This)->lpVtbl -> put_LogLevel(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_Start_Proxy( 
    IMasterServerAdministration __RPC_FAR * This);


void __RPC_STUB IMasterServerAdministration_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_Stop_Proxy( 
    IMasterServerAdministration __RPC_FAR * This);


void __RPC_STUB IMasterServerAdministration_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_get_BeaconListeners_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IMasterServerAdministration_get_BeaconListeners_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_put_Name_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMasterServerAdministration_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_put_Location_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMasterServerAdministration_put_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_put_AdminEmail_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMasterServerAdministration_put_AdminEmail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_get_Statistics_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [retval][out] */ IMasterServerAdministration_Statistics __RPC_FAR *pVal);


void __RPC_STUB IMasterServerAdministration_get_Statistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_get_Properties_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [retval][out] */ IMasterServerAdministration_PropertySet __RPC_FAR *pVal);


void __RPC_STUB IMasterServerAdministration_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_put_MOTD_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMasterServerAdministration_put_MOTD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_get_IsStarted_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IMasterServerAdministration_get_IsStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_get_LogLevel_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    /* [retval][out] */ byte __RPC_FAR *pVal);


void __RPC_STUB IMasterServerAdministration_get_LogLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMasterServerAdministration_put_LogLevel_Proxy( 
    IMasterServerAdministration __RPC_FAR * This,
    byte newVal);


void __RPC_STUB IMasterServerAdministration_put_LogLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMasterServerAdministration_INTERFACE_DEFINED__ */


#ifndef __IMasterServerEvents_INTERFACE_DEFINED__
#define __IMasterServerEvents_INTERFACE_DEFINED__

/* interface IMasterServerEvents */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IMasterServerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57129-D4E3-11D2-9018-004F4E006398")
    IMasterServerEvents : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IMasterServerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMasterServerEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMasterServerEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMasterServerEvents __RPC_FAR * This);
        
        END_INTERFACE
    } IMasterServerEventsVtbl;

    interface IMasterServerEvents
    {
        CONST_VTBL struct IMasterServerEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMasterServerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMasterServerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMasterServerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMasterServerEvents_INTERFACE_DEFINED__ */


#ifndef __IHost_INTERFACE_DEFINED__
#define __IHost_INTERFACE_DEFINED__

/* interface IHost */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B6B03A0C-D547-11D2-9018-004F4E006398")
    IHost : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Ping( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Hostname( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IP( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_IP( 
            DWORD newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Port( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Port( 
            long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_QueryPort( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_QueryPort( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Hops( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHost __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Ping )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Hostname )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IP )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IP )( 
            IHost __RPC_FAR * This,
            DWORD newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Port )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Port )( 
            IHost __RPC_FAR * This,
            long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QueryPort )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_QueryPort )( 
            IHost __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Hops )( 
            IHost __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IHost __RPC_FAR * This);
        
        END_INTERFACE
    } IHostVtbl;

    interface IHost
    {
        CONST_VTBL struct IHostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHost_get_Ping(This,pVal)	\
    (This)->lpVtbl -> get_Ping(This,pVal)

#define IHost_get_Hostname(This,pVal)	\
    (This)->lpVtbl -> get_Hostname(This,pVal)

#define IHost_get_IP(This,pVal)	\
    (This)->lpVtbl -> get_IP(This,pVal)

#define IHost_put_IP(This,newVal)	\
    (This)->lpVtbl -> put_IP(This,newVal)

#define IHost_get_Port(This,pVal)	\
    (This)->lpVtbl -> get_Port(This,pVal)

#define IHost_put_Port(This,newVal)	\
    (This)->lpVtbl -> put_Port(This,newVal)

#define IHost_get_QueryPort(This,pVal)	\
    (This)->lpVtbl -> get_QueryPort(This,pVal)

#define IHost_put_QueryPort(This,newVal)	\
    (This)->lpVtbl -> put_QueryPort(This,newVal)

#define IHost_get_Hops(This,pVal)	\
    (This)->lpVtbl -> get_Hops(This,pVal)

#define IHost_Update(This)	\
    (This)->lpVtbl -> Update(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_Ping_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IHost_get_Ping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_Hostname_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IHost_get_Hostname_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_IP_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IHost_get_IP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IHost_put_IP_Proxy( 
    IHost __RPC_FAR * This,
    DWORD newVal);


void __RPC_STUB IHost_put_IP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_Port_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IHost_get_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IHost_put_Port_Proxy( 
    IHost __RPC_FAR * This,
    long newVal);


void __RPC_STUB IHost_put_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_QueryPort_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IHost_get_QueryPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IHost_put_QueryPort_Proxy( 
    IHost __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IHost_put_QueryPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHost_get_Hops_Proxy( 
    IHost __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IHost_get_Hops_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHost_Update_Proxy( 
    IHost __RPC_FAR * This);


void __RPC_STUB IHost_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHost_INTERFACE_DEFINED__ */


#ifndef __IHostInfo_INTERFACE_DEFINED__
#define __IHostInfo_INTERFACE_DEFINED__

/* interface IHostInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHostInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D783C329-D613-11D2-9018-004F4E006398")
    IHostInfo : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateHeartbeat( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LastBeat( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_IP( 
            DWORD newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IP( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Port( 
            long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Port( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateClientSideObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHostInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHostInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHostInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHostInfo __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateHeartbeat )( 
            IHostInfo __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastBeat )( 
            IHostInfo __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IP )( 
            IHostInfo __RPC_FAR * This,
            DWORD newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IP )( 
            IHostInfo __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Port )( 
            IHostInfo __RPC_FAR * This,
            long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Port )( 
            IHostInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClientSideObject )( 
            IHostInfo __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        END_INTERFACE
    } IHostInfoVtbl;

    interface IHostInfo
    {
        CONST_VTBL struct IHostInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHostInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHostInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHostInfo_UpdateHeartbeat(This)	\
    (This)->lpVtbl -> UpdateHeartbeat(This)

#define IHostInfo_get_LastBeat(This,pVal)	\
    (This)->lpVtbl -> get_LastBeat(This,pVal)

#define IHostInfo_put_IP(This,newVal)	\
    (This)->lpVtbl -> put_IP(This,newVal)

#define IHostInfo_get_IP(This,pVal)	\
    (This)->lpVtbl -> get_IP(This,pVal)

#define IHostInfo_put_Port(This,newVal)	\
    (This)->lpVtbl -> put_Port(This,newVal)

#define IHostInfo_get_Port(This,pVal)	\
    (This)->lpVtbl -> get_Port(This,pVal)

#define IHostInfo_CreateClientSideObject(This,pVal)	\
    (This)->lpVtbl -> CreateClientSideObject(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHostInfo_UpdateHeartbeat_Proxy( 
    IHostInfo __RPC_FAR * This);


void __RPC_STUB IHostInfo_UpdateHeartbeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHostInfo_get_LastBeat_Proxy( 
    IHostInfo __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IHostInfo_get_LastBeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IHostInfo_put_IP_Proxy( 
    IHostInfo __RPC_FAR * This,
    DWORD newVal);


void __RPC_STUB IHostInfo_put_IP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHostInfo_get_IP_Proxy( 
    IHostInfo __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IHostInfo_get_IP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IHostInfo_put_Port_Proxy( 
    IHostInfo __RPC_FAR * This,
    long newVal);


void __RPC_STUB IHostInfo_put_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IHostInfo_get_Port_Proxy( 
    IHostInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IHostInfo_get_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHostInfo_CreateClientSideObject_Proxy( 
    IHostInfo __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IHostInfo_CreateClientSideObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHostInfo_INTERFACE_DEFINED__ */


#ifndef __IBeaconListenerEvents_INTERFACE_DEFINED__
#define __IBeaconListenerEvents_INTERFACE_DEFINED__

/* interface IBeaconListenerEvents */
/* [object][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBeaconListenerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDA57122-D4E3-11D2-9018-004F4E006398")
    IBeaconListenerEvents : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RegisterHostInfo( 
            IHostInfo __RPC_FAR *Game) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RevokeHostInfo( 
            IHostInfo __RPC_FAR *Game) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBeaconListenerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBeaconListenerEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBeaconListenerEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBeaconListenerEvents __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterHostInfo )( 
            IBeaconListenerEvents __RPC_FAR * This,
            IHostInfo __RPC_FAR *Game);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RevokeHostInfo )( 
            IBeaconListenerEvents __RPC_FAR * This,
            IHostInfo __RPC_FAR *Game);
        
        END_INTERFACE
    } IBeaconListenerEventsVtbl;

    interface IBeaconListenerEvents
    {
        CONST_VTBL struct IBeaconListenerEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBeaconListenerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBeaconListenerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBeaconListenerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBeaconListenerEvents_RegisterHostInfo(This,Game)	\
    (This)->lpVtbl -> RegisterHostInfo(This,Game)

#define IBeaconListenerEvents_RevokeHostInfo(This,Game)	\
    (This)->lpVtbl -> RevokeHostInfo(This,Game)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBeaconListenerEvents_RegisterHostInfo_Proxy( 
    IBeaconListenerEvents __RPC_FAR * This,
    IHostInfo __RPC_FAR *Game);


void __RPC_STUB IBeaconListenerEvents_RegisterHostInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBeaconListenerEvents_RevokeHostInfo_Proxy( 
    IBeaconListenerEvents __RPC_FAR * This,
    IHostInfo __RPC_FAR *Game);


void __RPC_STUB IBeaconListenerEvents_RevokeHostInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBeaconListenerEvents_INTERFACE_DEFINED__ */


#ifndef __IUplink_INTERFACE_DEFINED__
#define __IUplink_INTERFACE_DEFINED__

/* interface IUplink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IUplink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5FF85B6B-DE05-42B9-831F-DC6A4E30A513")
    IUplink : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HeartbeatInterval( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_HeartbeatInterval( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MasterServerName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MasterServerName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MasterServerPort( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MasterServerPort( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GameType( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_GameType( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerLocation( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerLocation( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerVersion( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerVersion( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MaxPlayers( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MaxPlayers( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_QueryPort( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_QueryPort( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GameName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_GameName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerPort( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerPort( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GameMode( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_GameMode( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerVersionMin( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ServerVersionMin( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPlayer( 
            BSTR Name) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindPlayer( 
            BSTR Name,
            /* [retval][out] */ IPlayer __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemovePlayer( 
            BSTR Name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_NumPlayers( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Players( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AdminName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AdminName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AdminEmail( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AdminEmail( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUplinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IUplink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IUplink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IUplink __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HeartbeatInterval )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HeartbeatInterval )( 
            IUplink __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MasterServerName )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MasterServerName )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MasterServerPort )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MasterServerPort )( 
            IUplink __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GameType )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GameType )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerName )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerName )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerLocation )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerLocation )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerVersion )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerVersion )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxPlayers )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxPlayers )( 
            IUplink __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QueryPort )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_QueryPort )( 
            IUplink __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GameName )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GameName )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerPort )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerPort )( 
            IUplink __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GameMode )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GameMode )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerVersionMin )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ServerVersionMin )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPlayer )( 
            IUplink __RPC_FAR * This,
            BSTR Name);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPlayer )( 
            IUplink __RPC_FAR * This,
            BSTR Name,
            /* [retval][out] */ IPlayer __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemovePlayer )( 
            IUplink __RPC_FAR * This,
            BSTR Name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumPlayers )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Players )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AdminName )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AdminName )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AdminEmail )( 
            IUplink __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AdminEmail )( 
            IUplink __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IUplink __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IUplink __RPC_FAR * This);
        
        END_INTERFACE
    } IUplinkVtbl;

    interface IUplink
    {
        CONST_VTBL struct IUplinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUplink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUplink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUplink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUplink_get_HeartbeatInterval(This,pVal)	\
    (This)->lpVtbl -> get_HeartbeatInterval(This,pVal)

#define IUplink_put_HeartbeatInterval(This,newVal)	\
    (This)->lpVtbl -> put_HeartbeatInterval(This,newVal)

#define IUplink_get_MasterServerName(This,pVal)	\
    (This)->lpVtbl -> get_MasterServerName(This,pVal)

#define IUplink_put_MasterServerName(This,newVal)	\
    (This)->lpVtbl -> put_MasterServerName(This,newVal)

#define IUplink_get_MasterServerPort(This,pVal)	\
    (This)->lpVtbl -> get_MasterServerPort(This,pVal)

#define IUplink_put_MasterServerPort(This,newVal)	\
    (This)->lpVtbl -> put_MasterServerPort(This,newVal)

#define IUplink_get_GameType(This,pVal)	\
    (This)->lpVtbl -> get_GameType(This,pVal)

#define IUplink_put_GameType(This,newVal)	\
    (This)->lpVtbl -> put_GameType(This,newVal)

#define IUplink_get_ServerName(This,pVal)	\
    (This)->lpVtbl -> get_ServerName(This,pVal)

#define IUplink_put_ServerName(This,newVal)	\
    (This)->lpVtbl -> put_ServerName(This,newVal)

#define IUplink_get_ServerLocation(This,pVal)	\
    (This)->lpVtbl -> get_ServerLocation(This,pVal)

#define IUplink_put_ServerLocation(This,newVal)	\
    (This)->lpVtbl -> put_ServerLocation(This,newVal)

#define IUplink_get_ServerVersion(This,pVal)	\
    (This)->lpVtbl -> get_ServerVersion(This,pVal)

#define IUplink_put_ServerVersion(This,newVal)	\
    (This)->lpVtbl -> put_ServerVersion(This,newVal)

#define IUplink_get_MaxPlayers(This,pVal)	\
    (This)->lpVtbl -> get_MaxPlayers(This,pVal)

#define IUplink_put_MaxPlayers(This,newVal)	\
    (This)->lpVtbl -> put_MaxPlayers(This,newVal)

#define IUplink_get_QueryPort(This,pVal)	\
    (This)->lpVtbl -> get_QueryPort(This,pVal)

#define IUplink_put_QueryPort(This,newVal)	\
    (This)->lpVtbl -> put_QueryPort(This,newVal)

#define IUplink_get_GameName(This,pVal)	\
    (This)->lpVtbl -> get_GameName(This,pVal)

#define IUplink_put_GameName(This,newVal)	\
    (This)->lpVtbl -> put_GameName(This,newVal)

#define IUplink_get_ServerPort(This,pVal)	\
    (This)->lpVtbl -> get_ServerPort(This,pVal)

#define IUplink_put_ServerPort(This,newVal)	\
    (This)->lpVtbl -> put_ServerPort(This,newVal)

#define IUplink_get_GameMode(This,pVal)	\
    (This)->lpVtbl -> get_GameMode(This,pVal)

#define IUplink_put_GameMode(This,newVal)	\
    (This)->lpVtbl -> put_GameMode(This,newVal)

#define IUplink_get_ServerVersionMin(This,pVal)	\
    (This)->lpVtbl -> get_ServerVersionMin(This,pVal)

#define IUplink_put_ServerVersionMin(This,newVal)	\
    (This)->lpVtbl -> put_ServerVersionMin(This,newVal)

#define IUplink_AddPlayer(This,Name)	\
    (This)->lpVtbl -> AddPlayer(This,Name)

#define IUplink_FindPlayer(This,Name,pVal)	\
    (This)->lpVtbl -> FindPlayer(This,Name,pVal)

#define IUplink_RemovePlayer(This,Name)	\
    (This)->lpVtbl -> RemovePlayer(This,Name)

#define IUplink_get_NumPlayers(This,pVal)	\
    (This)->lpVtbl -> get_NumPlayers(This,pVal)

#define IUplink_get_Players(This,pVal)	\
    (This)->lpVtbl -> get_Players(This,pVal)

#define IUplink_get_AdminName(This,pVal)	\
    (This)->lpVtbl -> get_AdminName(This,pVal)

#define IUplink_put_AdminName(This,newVal)	\
    (This)->lpVtbl -> put_AdminName(This,newVal)

#define IUplink_get_AdminEmail(This,pVal)	\
    (This)->lpVtbl -> get_AdminEmail(This,pVal)

#define IUplink_put_AdminEmail(This,newVal)	\
    (This)->lpVtbl -> put_AdminEmail(This,newVal)

#define IUplink_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IUplink_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_HeartbeatInterval_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_HeartbeatInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_HeartbeatInterval_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IUplink_put_HeartbeatInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_MasterServerName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_MasterServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_MasterServerName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_MasterServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_MasterServerPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_MasterServerPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_MasterServerPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IUplink_put_MasterServerPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_GameType_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_GameType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_GameType_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_GameType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_ServerName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_ServerName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_ServerLocation_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_ServerLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_ServerLocation_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_ServerLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_ServerVersion_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_ServerVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_ServerVersion_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_ServerVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_MaxPlayers_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_MaxPlayers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_MaxPlayers_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB IUplink_put_MaxPlayers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_QueryPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_QueryPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_QueryPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB IUplink_put_QueryPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_GameName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_GameName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_GameName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_GameName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_ServerPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_ServerPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_ServerPort_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB IUplink_put_ServerPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_GameMode_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_GameMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_GameMode_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_GameMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_ServerVersionMin_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_ServerVersionMin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_ServerVersionMin_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_ServerVersionMin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IUplink_AddPlayer_Proxy( 
    IUplink __RPC_FAR * This,
    BSTR Name);


void __RPC_STUB IUplink_AddPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IUplink_FindPlayer_Proxy( 
    IUplink __RPC_FAR * This,
    BSTR Name,
    /* [retval][out] */ IPlayer __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IUplink_FindPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IUplink_RemovePlayer_Proxy( 
    IUplink __RPC_FAR * This,
    BSTR Name);


void __RPC_STUB IUplink_RemovePlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_NumPlayers_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_NumPlayers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_Players_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IUplink_get_Players_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_AdminName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_AdminName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_AdminName_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_AdminName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IUplink_get_AdminEmail_Proxy( 
    IUplink __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUplink_get_AdminEmail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IUplink_put_AdminEmail_Proxy( 
    IUplink __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUplink_put_AdminEmail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IUplink_Start_Proxy( 
    IUplink __RPC_FAR * This);


void __RPC_STUB IUplink_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IUplink_Stop_Proxy( 
    IUplink __RPC_FAR * This);


void __RPC_STUB IUplink_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUplink_INTERFACE_DEFINED__ */


#ifndef __IProvideGUID_INTERFACE_DEFINED__
#define __IProvideGUID_INTERFACE_DEFINED__

/* interface IProvideGUID */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IProvideGUID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F70CCCE-D716-11d2-9018-004F4E006398")
    IProvideGUID : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GUID( 
            /* [retval][out] */ GUID __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_GUID( 
            GUID newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideGUIDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProvideGUID __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProvideGUID __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProvideGUID __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GUID )( 
            IProvideGUID __RPC_FAR * This,
            /* [retval][out] */ GUID __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GUID )( 
            IProvideGUID __RPC_FAR * This,
            GUID newVal);
        
        END_INTERFACE
    } IProvideGUIDVtbl;

    interface IProvideGUID
    {
        CONST_VTBL struct IProvideGUIDVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideGUID_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideGUID_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideGUID_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideGUID_get_GUID(This,pVal)	\
    (This)->lpVtbl -> get_GUID(This,pVal)

#define IProvideGUID_put_GUID(This,newVal)	\
    (This)->lpVtbl -> put_GUID(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IProvideGUID_get_GUID_Proxy( 
    IProvideGUID __RPC_FAR * This,
    /* [retval][out] */ GUID __RPC_FAR *pVal);


void __RPC_STUB IProvideGUID_get_GUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IProvideGUID_put_GUID_Proxy( 
    IProvideGUID __RPC_FAR * This,
    GUID newVal);


void __RPC_STUB IProvideGUID_put_GUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideGUID_INTERFACE_DEFINED__ */


#ifndef __IListViewItem_INTERFACE_DEFINED__
#define __IListViewItem_INTERFACE_DEFINED__

/* interface IListViewItem */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IListViewItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1386AE5C-D6F3-11d2-9018-004F4E006398")
    IListViewItem : public IProvideGUID
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetColumnText( 
            int nColumn,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IListViewItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IListViewItem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IListViewItem __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IListViewItem __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GUID )( 
            IListViewItem __RPC_FAR * This,
            /* [retval][out] */ GUID __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GUID )( 
            IListViewItem __RPC_FAR * This,
            GUID newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnText )( 
            IListViewItem __RPC_FAR * This,
            int nColumn,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IListViewItemVtbl;

    interface IListViewItem
    {
        CONST_VTBL struct IListViewItemVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IListViewItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IListViewItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IListViewItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IListViewItem_get_GUID(This,pVal)	\
    (This)->lpVtbl -> get_GUID(This,pVal)

#define IListViewItem_put_GUID(This,newVal)	\
    (This)->lpVtbl -> put_GUID(This,newVal)


#define IListViewItem_GetColumnText(This,nColumn,pVal)	\
    (This)->lpVtbl -> GetColumnText(This,nColumn,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IListViewItem_GetColumnText_Proxy( 
    IListViewItem __RPC_FAR * This,
    int nColumn,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IListViewItem_GetColumnText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IListViewItem_INTERFACE_DEFINED__ */


#ifndef __IListViewItemContainer_INTERFACE_DEFINED__
#define __IListViewItemContainer_INTERFACE_DEFINED__

/* interface IListViewItemContainer */
/* [object][helpstring][uuid] */ 

typedef /* [public][public] */ struct __MIDL_IListViewItemContainer_0001
    {
    int fmt;
    int cx;
    BSTR Text;
    }	IListViewItemContainer_ColumnInfo;


EXTERN_C const IID IID_IListViewItemContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1386AE5A-D6F3-11d2-9018-004F4E006398")
    IListViewItemContainer : public IListViewItem
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ListViewColumnCount( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ListViewColumnInfo( 
            int nIndex,
            /* [retval][out] */ IListViewItemContainer_ColumnInfo __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RefreshItems( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Window( 
            DWORD newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IListViewItemContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IListViewItemContainer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IListViewItemContainer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IListViewItemContainer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GUID )( 
            IListViewItemContainer __RPC_FAR * This,
            /* [retval][out] */ GUID __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GUID )( 
            IListViewItemContainer __RPC_FAR * This,
            GUID newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnText )( 
            IListViewItemContainer __RPC_FAR * This,
            int nColumn,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ListViewColumnCount )( 
            IListViewItemContainer __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ListViewColumnInfo )( 
            IListViewItemContainer __RPC_FAR * This,
            int nIndex,
            /* [retval][out] */ IListViewItemContainer_ColumnInfo __RPC_FAR *pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumItems )( 
            IListViewItemContainer __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RefreshItems )( 
            IListViewItemContainer __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Window )( 
            IListViewItemContainer __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Window )( 
            IListViewItemContainer __RPC_FAR * This,
            DWORD newVal);
        
        END_INTERFACE
    } IListViewItemContainerVtbl;

    interface IListViewItemContainer
    {
        CONST_VTBL struct IListViewItemContainerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IListViewItemContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IListViewItemContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IListViewItemContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IListViewItemContainer_get_GUID(This,pVal)	\
    (This)->lpVtbl -> get_GUID(This,pVal)

#define IListViewItemContainer_put_GUID(This,newVal)	\
    (This)->lpVtbl -> put_GUID(This,newVal)


#define IListViewItemContainer_GetColumnText(This,nColumn,pVal)	\
    (This)->lpVtbl -> GetColumnText(This,nColumn,pVal)


#define IListViewItemContainer_get_ListViewColumnCount(This,pVal)	\
    (This)->lpVtbl -> get_ListViewColumnCount(This,pVal)

#define IListViewItemContainer_get_ListViewColumnInfo(This,nIndex,pVal)	\
    (This)->lpVtbl -> get_ListViewColumnInfo(This,nIndex,pVal)

#define IListViewItemContainer_EnumItems(This,ppenum)	\
    (This)->lpVtbl -> EnumItems(This,ppenum)

#define IListViewItemContainer_RefreshItems(This)	\
    (This)->lpVtbl -> RefreshItems(This)

#define IListViewItemContainer_get_Window(This,pVal)	\
    (This)->lpVtbl -> get_Window(This,pVal)

#define IListViewItemContainer_put_Window(This,newVal)	\
    (This)->lpVtbl -> put_Window(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_get_ListViewColumnCount_Proxy( 
    IListViewItemContainer __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB IListViewItemContainer_get_ListViewColumnCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_get_ListViewColumnInfo_Proxy( 
    IListViewItemContainer __RPC_FAR * This,
    int nIndex,
    /* [retval][out] */ IListViewItemContainer_ColumnInfo __RPC_FAR *pVal);


void __RPC_STUB IListViewItemContainer_get_ListViewColumnInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_EnumItems_Proxy( 
    IListViewItemContainer __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IListViewItemContainer_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_RefreshItems_Proxy( 
    IListViewItemContainer __RPC_FAR * This);


void __RPC_STUB IListViewItemContainer_RefreshItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_get_Window_Proxy( 
    IListViewItemContainer __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IListViewItemContainer_get_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IListViewItemContainer_put_Window_Proxy( 
    IListViewItemContainer __RPC_FAR * This,
    DWORD newVal);


void __RPC_STUB IListViewItemContainer_put_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IListViewItemContainer_INTERFACE_DEFINED__ */


#ifndef __IListView_INTERFACE_DEFINED__
#define __IListView_INTERFACE_DEFINED__

/* interface IListView */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IListView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1386AE5D-D6F3-11d2-9018-004F4E006398")
    IListView : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Container( 
            /* [retval][out] */ IListViewItemContainer __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Container( 
            IListViewItemContainer __RPC_FAR *newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IListViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IListView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IListView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IListView __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Container )( 
            IListView __RPC_FAR * This,
            /* [retval][out] */ IListViewItemContainer __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Container )( 
            IListView __RPC_FAR * This,
            IListViewItemContainer __RPC_FAR *newVal);
        
        END_INTERFACE
    } IListViewVtbl;

    interface IListView
    {
        CONST_VTBL struct IListViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IListView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IListView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IListView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IListView_get_Container(This,pVal)	\
    (This)->lpVtbl -> get_Container(This,pVal)

#define IListView_put_Container(This,newVal)	\
    (This)->lpVtbl -> put_Container(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IListView_get_Container_Proxy( 
    IListView __RPC_FAR * This,
    /* [retval][out] */ IListViewItemContainer __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IListView_get_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IListView_put_Container_Proxy( 
    IListView __RPC_FAR * This,
    IListViewItemContainer __RPC_FAR *newVal);


void __RPC_STUB IListView_put_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IListView_INTERFACE_DEFINED__ */


#ifndef __IObjectManager_INTERFACE_DEFINED__
#define __IObjectManager_INTERFACE_DEFINED__

/* interface IObjectManager */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IObjectManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DF360EF0-DE0D-11d2-9023-004F4E006398")
    IObjectManager : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BindToObject( 
            const GUID __RPC_FAR *pguid,
            REFIID riid,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *p) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumObjects( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AbsName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Parent( 
            IObjectManager __RPC_FAR *newVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IObjectManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IObjectManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )( 
            IObjectManager __RPC_FAR * This,
            const GUID __RPC_FAR *pguid,
            REFIID riid,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *p);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumObjects )( 
            IObjectManager __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IObjectManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AbsName )( 
            IObjectManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Parent )( 
            IObjectManager __RPC_FAR * This,
            IObjectManager __RPC_FAR *newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            IObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IObjectManager __RPC_FAR * This);
        
        END_INTERFACE
    } IObjectManagerVtbl;

    interface IObjectManager
    {
        CONST_VTBL struct IObjectManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectManager_BindToObject(This,pguid,riid,p)	\
    (This)->lpVtbl -> BindToObject(This,pguid,riid,p)

#define IObjectManager_EnumObjects(This,ppenum)	\
    (This)->lpVtbl -> EnumObjects(This,ppenum)

#define IObjectManager_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IObjectManager_get_AbsName(This,pVal)	\
    (This)->lpVtbl -> get_AbsName(This,pVal)

#define IObjectManager_put_Parent(This,newVal)	\
    (This)->lpVtbl -> put_Parent(This,newVal)

#define IObjectManager_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IObjectManager_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IObjectManager_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectManager_BindToObject_Proxy( 
    IObjectManager __RPC_FAR * This,
    const GUID __RPC_FAR *pguid,
    REFIID riid,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *p);


void __RPC_STUB IObjectManager_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectManager_EnumObjects_Proxy( 
    IObjectManager __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IObjectManager_EnumObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IObjectManager_get_Name_Proxy( 
    IObjectManager __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IObjectManager_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IObjectManager_get_AbsName_Proxy( 
    IObjectManager __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IObjectManager_get_AbsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IObjectManager_put_Parent_Proxy( 
    IObjectManager __RPC_FAR * This,
    IObjectManager __RPC_FAR *newVal);


void __RPC_STUB IObjectManager_put_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectManager_Refresh_Proxy( 
    IObjectManager __RPC_FAR * This);


void __RPC_STUB IObjectManager_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectManager_Start_Proxy( 
    IObjectManager __RPC_FAR * This);


void __RPC_STUB IObjectManager_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectManager_Stop_Proxy( 
    IObjectManager __RPC_FAR * This);


void __RPC_STUB IObjectManager_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectManager_INTERFACE_DEFINED__ */


#ifndef __IServerObjectManager_INTERFACE_DEFINED__
#define __IServerObjectManager_INTERFACE_DEFINED__

/* interface IServerObjectManager */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IServerObjectManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("90EFD9F9-D7A8-11D2-901A-004F4E006398")
    IServerObjectManager : public IObjectManager
    {
    public:
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_HostAddress( 
            BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServerObjectManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IServerObjectManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IServerObjectManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IServerObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )( 
            IServerObjectManager __RPC_FAR * This,
            const GUID __RPC_FAR *pguid,
            REFIID riid,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *p);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumObjects )( 
            IServerObjectManager __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IServerObjectManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AbsName )( 
            IServerObjectManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Parent )( 
            IServerObjectManager __RPC_FAR * This,
            IObjectManager __RPC_FAR *newVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            IServerObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IServerObjectManager __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IServerObjectManager __RPC_FAR * This);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HostAddress )( 
            IServerObjectManager __RPC_FAR * This,
            BSTR newVal);
        
        END_INTERFACE
    } IServerObjectManagerVtbl;

    interface IServerObjectManager
    {
        CONST_VTBL struct IServerObjectManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServerObjectManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IServerObjectManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IServerObjectManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IServerObjectManager_BindToObject(This,pguid,riid,p)	\
    (This)->lpVtbl -> BindToObject(This,pguid,riid,p)

#define IServerObjectManager_EnumObjects(This,ppenum)	\
    (This)->lpVtbl -> EnumObjects(This,ppenum)

#define IServerObjectManager_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IServerObjectManager_get_AbsName(This,pVal)	\
    (This)->lpVtbl -> get_AbsName(This,pVal)

#define IServerObjectManager_put_Parent(This,newVal)	\
    (This)->lpVtbl -> put_Parent(This,newVal)

#define IServerObjectManager_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IServerObjectManager_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IServerObjectManager_Stop(This)	\
    (This)->lpVtbl -> Stop(This)


#define IServerObjectManager_put_HostAddress(This,newVal)	\
    (This)->lpVtbl -> put_HostAddress(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IServerObjectManager_put_HostAddress_Proxy( 
    IServerObjectManager __RPC_FAR * This,
    BSTR newVal);


void __RPC_STUB IServerObjectManager_put_HostAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IServerObjectManager_INTERFACE_DEFINED__ */



#ifndef __GNETCORELib_LIBRARY_DEFINED__
#define __GNETCORELib_LIBRARY_DEFINED__

/* library GNETCORELib */
/* [helpstring][version][uuid] */ 





















EXTERN_C const IID LIBID_GNETCORELib;
#endif /* __GNETCORELib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_Core_0263 */
/* [local] */ 

#ifdef __cplusplus
#include <comdef.h>
struct __declspec(uuid("9DA94F0C-D4EB-11d2-9018-004F4E006398")) CATID_GNet_BeaconListeners;
#endif


extern RPC_IF_HANDLE __MIDL_itf_Core_0263_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_Core_0263_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


