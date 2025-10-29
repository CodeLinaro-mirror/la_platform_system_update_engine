/* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*
*    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <update_engine/update_engine_service.h>
#include <update_engine/apply_payload.h>
#include <stdlib.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <binder/TextOutput.h>
#include <base/run_loop.h>
#include <base/threading/thread.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <base/logging.h>

using namespace android;

namespace android {


/* apply payload interface implementation called by client */
class BpStreamUpdateService : public BpInterface <IStreamUpdateService> {
    public :

        BpStreamUpdateService (const sp<IBinder>& binderObj) : BpInterface <IStreamUpdateService> (binderObj) {
        }

        virtual void applyUpdatePayload(const string& payload,
                        int64_t payload_offset,
                        int64_t payload_size,
                        const vector<string>& headers,
                        int64_t status_fd) {
            LOG(INFO) << "\n Update_engine_client payload url: " << payload.c_str();
            LOG(INFO) << "\n Update_engine_client payload offset: " << payload_offset;
            LOG(INFO) << "\n Update_engine_client payload size: " << payload_size;
            Parcel data, reply;
            data.writeInterfaceToken(IStreamUpdateService::getInterfaceDescriptor());
            data.writeString16(String16(payload.c_str()));
            data.writeCString(payload.c_str());
            data.writeInt64(payload_offset);
            data.writeInt64(payload_size);

            for(size_t i = 0; i < headers.size(); ++i) {
              LOG(INFO) << "test headers strings split :"<< headers[i];
              data.writeCString(headers[i].c_str());
            }
            remote()->transact(APPLYPAYLOAD, data, &reply, IBinder::FLAG_ONEWAY);    // asynchronous call

            return;
        }

        virtual void registerCallback(sp<IBinder> &binder)
        {
          Parcel data, reply;
          data.writeInterfaceToken(IStreamUpdateService::getInterfaceDescriptor());
          data.writeStrongBinder(binder);
          remote()->transact(TRANSACTION_REGISTER_CALLBACK, data, &reply,IBinder::FLAG_ONEWAY);
          return;
        }

};

class StreamUpdateService : public BnStreamUpdateService {
    virtual void applyUpdatePayload(const string& payload,
                        int64_t payload_offset,
                        int64_t payload_size,
                        const vector<string>& headers,
                        int64_t status_fd){
       LOG(INFO) << "\n Update_engine_client payload url: " << payload.c_str();
       LOG(INFO) << "\n Update_engine_client payload offset: " <<payload_offset;
       LOG(INFO) << "\n Update_engine_client payload statusfd: " << status_fd;
       return;
    }
    virtual void registerCallback(sp<IBinder> &binder)
    {
      Mutex::Autolock _l(mLock);
      sp<IStreamUpdateNotifyService> callback = interface_cast<IStreamUpdateNotifyService>(binder);
      mCallbacks.push(callback);
      LOG(INFO) << "Service does registerCallback ";
      return;
    }

};
IMPLEMENT_META_INTERFACE(StreamUpdateService, "StreamUpdateService");


/* implementation of interface exposed by binder IStreamUpdateService interface service side (ontrasact function) */
status_t BnStreamUpdateService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){
    LOG(INFO) << "inside BnStreamUpdateService::onTransact code="<<code<<"\n";
    data.checkInterface(this);
    switch(code) {
        case APPLYPAYLOAD: {
            LOG(INFO) << " APPLYPAYLOAD";
            String16 payload(data.readString16());
            const char* payload_2(data.readCString());
            if( payload_2 == NULL) {
              printf("Error couldnot allocate for payload_2\n");
              return NULL;
            }
            int64_t payload_offset = data.readInt64();
            int64_t payload_size = data.readInt64();
            const char* payload_file_hash(data.readCString());
            if( payload_file_hash == NULL ) {
              printf("Error couldnot allocate for payload_file_hash\n");
              return NULL;
            }
            const char* payload_file_size(data.readCString());
            if( payload_file_size == NULL ) {
              printf("Error couldnot allocate for payload_file_size\n");
              return NULL;
            }
            const char* payload_metadata_hash(data.readCString());
            if( payload_metadata_hash == NULL ) {
              printf("Error couldnot allocate for payload_metadata_hash\n");
              return NULL;
            }
            const char* payload_metadata_size(data.readCString());
            if( payload_metadata_size == NULL ) {
              printf("Error couldnot allocate for payload_metadata_size\n");
              return NULL;
            }
            int64_t update_status_fd(-1);
            LOG(INFO) << " APPLYPAYLOAD payload url " << payload_2;
            LOG(INFO) << " payload_file_hash " << payload_file_hash;
            LOG(INFO) << " payload_file_size " <<  payload_file_size;
            LOG(INFO) << " payload_metadata_hash " << payload_metadata_hash;
            LOG(INFO) << " payload_metadata_size " << payload_metadata_size;
            string str(payload_2);
            LOG(INFO) << " APPLYPAYLOAD c++ string payload url " << str.c_str();
            LOG(INFO) << " APPLYPAYLOAD payload_offset" << payload_offset;
            LOG(INFO) << " payload_size: " << payload_size;
            vector<string> header_init;
            header_init.push_back(payload_file_hash);
            header_init.push_back(payload_file_size);
            header_init.push_back(payload_metadata_hash);
            header_init.push_back(payload_metadata_size);
            for(size_t i = 0; i < header_init.size(); ++i) {
              LOG(INFO) << " header_init :"<< header_init[i];
            }
            chromeos_update_engine::ApplyPayload* payload_obj = new chromeos_update_engine::ApplyPayload();
            payload_obj->ApplyUpdatePayload(
               str, payload_offset, payload_size, header_init, update_status_fd);
            LOG(INFO) << "\n  applypayload done ";
            delete payload_obj;
            payload_obj = nullptr;
            for (int i = mCallbacks.size() - 1; i >= 0; i--)
            {
              LOG(INFO) << " execute callback ";
              sp<IStreamUpdateNotifyService> cb = mCallbacks[i];
              cb->triggerNotify();
              mCallbacks.removeAt(i);
            }
            return NO_ERROR;
        } break;
        case TRANSACTION_REGISTER_CALLBACK :
        {
          LOG(INFO) << " TRANSACTION_REGISTER_CALLBACK read binder object ";
          sp<IBinder> service = data.readStrongBinder();
          registerCallback(service);
          return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }

};


class BpStreamUpdateNotifyService: public BpInterface<IStreamUpdateNotifyService>
{
public:
    BpStreamUpdateNotifyService(const sp<IBinder>& impl)
        : BpInterface<IStreamUpdateNotifyService>(impl)
    {
    }

    virtual void triggerNotify()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IStreamUpdateNotifyService::getInterfaceDescriptor());
        remote()->transact(BnStreamUpdateNotifyService::TRANSACTION_NOTIFY, data, &reply);
        return;
    }
};

IMPLEMENT_META_INTERFACE(StreamUpdateNotifyService, "StreamUpdateNotifyService");

status_t BnStreamUpdateNotifyService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case TRANSACTION_NOTIFY: {
            CHECK_INTERFACE(IStreamUpdateNotifyService, data, reply);
            triggerNotify();
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
    return NO_ERROR;
}


UpdateNotifyService::UpdateNotifyService()
{
}

UpdateNotifyService::~UpdateNotifyService()
{
}

void UpdateNotifyService::triggerNotify()
{
    LOG(INFO) << " UpdateNotifyService:: triggered  ";
    Mutex::Autolock _l(mNotifyLock);
    mNotifyCond.signal();
    LOG(INFO) << "UpdateNotifyService:: notification sent ";
}
};


void stream_update_service_init() {
    defaultServiceManager()->addService(String16("StreamUpdateService"), new StreamUpdateService());
    android::ProcessState::self()->startThreadPool();
    auto thread = IPCThreadState::self();
    if (thread == NULL) {
       printf("Error in self() call\n");
       return;
    }
    LOG(INFO) << "StreamUpdateService service is added";
    thread->joinThreadPool();
    LOG(INFO) << "StreamUpdateService service thread joined";
}

sp<IStreamUpdateService> getStreamUpdateService() {
   sp<IServiceManager> servicemanager_inst = defaultServiceManager();
   if (servicemanager_inst == NULL) {
     printf("Error couldnot get service manager, abort");
     return NULL;
   }
   sp<IBinder> binder = servicemanager_inst->getService(String16("StreamUpdateService"));
   if (binder == NULL) {
     printf("Error couldnot get binder inst from service manager, abort");
     return NULL;
   }
   sp<IStreamUpdateService> stream_update_service = interface_cast<IStreamUpdateService>(binder);
   return stream_update_service;
}

