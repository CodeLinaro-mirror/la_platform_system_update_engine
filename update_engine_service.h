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

#include <binder/IInterface.h>
#include <string>
#include <vector>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <base/logging.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

using std::string;
using std::vector;

using namespace android;

namespace android {


class IStreamUpdateNotifyService: public IInterface {
public:
    DECLARE_META_INTERFACE(StreamUpdateNotifyService);

    virtual void triggerNotify() = 0;
};

class BnStreamUpdateNotifyService: public BnInterface<IStreamUpdateNotifyService> {
public:
    enum {
        TRANSACTION_NOTIFY = IBinder::FIRST_CALL_TRANSACTION + 3,
    };

    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flag = 0);
};

class UpdateNotifyService: public BnStreamUpdateNotifyService
{
public:
    UpdateNotifyService();
    ~UpdateNotifyService();
    virtual void triggerNotify();
    mutable Condition mNotifyCond;
    mutable Mutex mNotifyLock;
};



class IStreamUpdateService : public IInterface {
  public:
    virtual void applyUpdatePayload(const string& payload,
                        int64_t payload_offset,
                        int64_t payload_size,
                        const vector<string>& headers,
                        int64_t status_fd) = 0;
    virtual void registerCallback(sp<IBinder>& binder) = 0;
    enum {
        APPLYPAYLOAD = IBinder::FIRST_CALL_TRANSACTION,
        TRANSACTION_REGISTER_CALLBACK = IBinder::FIRST_CALL_TRANSACTION + 1 ,
    };


    DECLARE_META_INTERFACE(StreamUpdateService);
    pthread_mutex_t m_serverWaitMutex;
    pthread_cond_t m_serverWaitCond;
    mutable Mutex mLock;
    Vector<sp<IStreamUpdateNotifyService>>  mCallbacks;
};

class BnStreamUpdateService : public BnInterface <IStreamUpdateService> {
  public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);

    Vector<sp<IStreamUpdateNotifyService>> mCallbacks;
};

};

void stream_update_service_init();
sp<IStreamUpdateService> getStreamUpdateService();
