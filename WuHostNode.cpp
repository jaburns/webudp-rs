#include "WuHost.h"
#include <arpa/inet.h>
#include <nan.h>
#include <unordered_map>

const char* const kInvalidArgumentCountErr = "Wrong number of arguments";
const char* const kInvalidArguments = "Invalid arguments";

struct Address {
  Address(uint32_t addr, uint16_t port) : address(addr), port(port) {
    snprintf(textAddress, sizeof(textAddress), "%u.%u.%u.%u", (addr & 0xFF000000) >> 24,
             (addr & 0x00FF0000) >> 16, (addr & 0x0000FF00) >> 8, (addr & 0x000000FF));
  }

  uint32_t address;
  uint16_t port;
  char textAddress[16];
};

struct WuHost {
  Wu* wu = nullptr;
};

class WuHostWrap : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init);

  explicit WuHostWrap(Wu* wu);
  ~WuHostWrap();

  static NAN_METHOD(New);
  static NAN_METHOD(ExchangeSDP);
  static NAN_METHOD(SetUDPWriteFunction);
  static NAN_METHOD(HandleUDP);
  static NAN_METHOD(Serve);
  static NAN_METHOD(SetClientJoinFunction);
  static NAN_METHOD(SetClientLeaveFunction);
  static NAN_METHOD(SetTextDataReceivedFunction);
  static NAN_METHOD(SetBinaryDataReceivedFunction);
  static NAN_METHOD(RemoveClient);
  static NAN_METHOD(SendTextData);
  static NAN_METHOD(SendBinaryData);
  static Nan::Persistent<v8::Function> constructor;

  std::unordered_map<uint32_t, WuClient*> clients;
  uint32_t idCounter = 1;
  WuHost host;
  Nan::Callback udpWriteCallback;
  Nan::Callback clientJoinCallback;
  Nan::Callback clientLeaveCallback;
  Nan::Callback textDataCallback;
  Nan::Callback binaryDataCallback;

  void HandleClientJoin(WuClient* client);
  void HandleClientLeave(WuClient* client);
  void HandleContent(const WuEvent* evt);

  void RemoveClient(uint32_t id);
};

void WuHostWrap::HandleClientJoin(WuClient* client) {
  uint32_t id = (uint32_t)(uintptr_t) WuClientGetUserData(client);

  if (!id) {
    id = idCounter++;
    WuClientSetUserData(client, (void*) (uintptr_t) id);
    clients[id] = client;
  }

  WuAddress address = WuClientGetAddress(client);
  Address remote(address.host, address.port);

  auto args = Nan::New<v8::Object>();
  Nan::Set(args, Nan::New("address").ToLocalChecked(),
           Nan::New(remote.textAddress).ToLocalChecked());
  Nan::Set(args, Nan::New("port").ToLocalChecked(), Nan::New(remote.port));
  Nan::Set(args, Nan::New("clientId").ToLocalChecked(), Nan::New(id));

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {args};

  Nan::AsyncResource ar("ClientJoin");
  clientJoinCallback.Call(argc, argv, &ar);
}

void WuHostWrap::HandleClientLeave(WuClient* client) {
  uintptr_t id = (uintptr_t) WuClientGetUserData(client);
  WuAddress address = WuClientGetAddress(client);
  Address remote(address.host, address.port);

  auto args = Nan::New<v8::Object>();
  Nan::Set(args, Nan::New("address").ToLocalChecked(),
           Nan::New(remote.textAddress).ToLocalChecked());
  Nan::Set(args, Nan::New("port").ToLocalChecked(), Nan::New(remote.port));
  Nan::Set(args, Nan::New("clientId").ToLocalChecked(), Nan::New((uint32_t) id));

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {args};

  Nan::AsyncResource ar("ClientLeave");
  clientLeaveCallback.Call(argc, argv, &ar);

  WuRemoveClient(host.wu, client);
}

void WuHostWrap::HandleContent(const WuEvent* evt) {
  uintptr_t id = (uintptr_t) WuClientGetUserData(evt->client);

  WuAddress address = WuClientGetAddress(evt->client);
  Address remote(address.host, address.port);

  auto args = Nan::New<v8::Object>();
  Nan::Set(args, Nan::New("address").ToLocalChecked(),
           Nan::New(remote.textAddress).ToLocalChecked());
  Nan::Set(args, Nan::New("port").ToLocalChecked(), Nan::New(remote.port));
  Nan::Set(args, Nan::New("clientId").ToLocalChecked(), Nan::New((uint32_t) id));

  const int argc = 1;

  if (evt->type == WuEvent_TextData) {
    Nan::Set(args, Nan::New("text").ToLocalChecked(),
             Nan::New((const char*) evt->data, evt->length).ToLocalChecked());
    v8::Local<v8::Value> argv[argc] = {args};

    Nan::AsyncResource ar("TextData");
    textDataCallback.Call(argc, argv, &ar);
  } else if (evt->type == WuEvent_BinaryData) {
    auto buf = Nan::CopyBuffer((const char*) evt->data, (uint32_t) evt->length).ToLocalChecked();
    Nan::Set(args, Nan::New("data").ToLocalChecked(), buf);
    v8::Local<v8::Value> argv[argc] = {args};

    Nan::AsyncResource ar("BinaryData");
    binaryDataCallback.Call(argc, argv, &ar);
  }
}

void WriteUDPData(const uint8_t* data, size_t length, const WuClient* client, void* userData) {
  WuHostWrap* wrap = (WuHostWrap*) userData;

  WuAddress address = WuClientGetAddress(client);

  Address remote(address.host, address.port);

  auto buf = Nan::CopyBuffer((const char*) data, (uint32_t) length).ToLocalChecked();

  auto addr = Nan::New<v8::Object>();
  Nan::Set(addr, Nan::New("address").ToLocalChecked(),
           Nan::New(remote.textAddress).ToLocalChecked());
  Nan::Set(addr, Nan::New("port").ToLocalChecked(), Nan::New(remote.port));

  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = {buf, addr};

  Nan::AsyncResource ar("UDPWrite");
  wrap->udpWriteCallback.Call(argc, argv, &ar);
}

void WuHostWrap::RemoveClient(uint32_t id) {
  if (clients.count(id) > 0) {
    WuClient* client = clients[id];
    clients.erase(id);
    WuRemoveClient(host.wu, client);
  }
}

Nan::Persistent<v8::Function> WuHostWrap::constructor;

NAN_MODULE_INIT(WuHostWrap::Init) {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Host").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "exchangeSDP", ExchangeSDP);
  Nan::SetPrototypeMethod(tpl, "setUDPWriteFunction", SetUDPWriteFunction);
  Nan::SetPrototypeMethod(tpl, "handleUDP", HandleUDP);
  Nan::SetPrototypeMethod(tpl, "serve", Serve);
  Nan::SetPrototypeMethod(tpl, "onClientJoin", SetClientJoinFunction);
  Nan::SetPrototypeMethod(tpl, "onClientLeave", SetClientLeaveFunction);
  Nan::SetPrototypeMethod(tpl, "onTextData", SetTextDataReceivedFunction);
  Nan::SetPrototypeMethod(tpl, "onBinaryData", SetBinaryDataReceivedFunction);
  Nan::SetPrototypeMethod(tpl, "removeClient", RemoveClient);
  Nan::SetPrototypeMethod(tpl, "sendText", SendTextData);
  Nan::SetPrototypeMethod(tpl, "sendBinary", SendBinaryData);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Host").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

WuHostWrap::WuHostWrap(Wu* wu) { host.wu = wu; }
WuHostWrap::~WuHostWrap() { free(host.wu); }

NAN_METHOD(WuHostWrap::New) {
  if (info.Length() < 2) {
    Nan::ThrowError(kInvalidArgumentCountErr);
    return;
  }

  if (info.IsConstructCall()) {
    int maxClients = 512;
    if (info.Length() >= 3 && info[2]->IsObject()) {
      Nan::MaybeLocal<v8::Object> maybeExtraArgs = Nan::To<v8::Object>(info[2]);

      if (!maybeExtraArgs.IsEmpty()) {
        v8::Local<v8::Object> extraArgs = maybeExtraArgs.ToLocalChecked();

        v8::MaybeLocal<v8::Value> maybeMaxClientsVal =
            Nan::Get(extraArgs, Nan::New("maxClients").ToLocalChecked());

        if (!maybeMaxClientsVal.IsEmpty() && maybeMaxClientsVal.ToLocalChecked()->IsNumber()) {
          Nan::Maybe<int32_t> maybeMaxClients =
              Nan::To<int32_t>(maybeMaxClientsVal.ToLocalChecked());

          if (maybeMaxClients.IsJust()) {
            maxClients = maybeMaxClients.FromJust();

            if (maxClients <= 0) {
              maxClients = 1;
            }
          }
        }
      }
    }

    Nan::Utf8String host(info[0]);
    Nan::Utf8String port(info[1]);

    Wu* wu = nullptr;
    int32_t status = WuCreate(*host, *port, maxClients, &wu);

    if (status != WU_OK) {
      Nan::ThrowError("Initialization error");
      return;
    }

    WuHostWrap* obj = new WuHostWrap(wu);
    WuSetUserData(wu, obj);
    WuSetUDPWriteFunction(wu, WriteUDPData);

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = {info[0], info[1]};
    v8::Local<v8::Function> cons = Nan::New(constructor);
    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(WuHostWrap::ExchangeSDP) {
  if (info.Length() < 1) {
    Nan::ThrowError(kInvalidArgumentCountErr);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  Wu* wu = obj->host.wu;

  Nan::Utf8String sdp(info[0]);
  const SDPResult res = WuExchangeSDP(wu, *sdp, sdp.length());

  if (res.status == WuSDPStatus_Success) {
    Nan::MaybeLocal<v8::String> responseSdp = Nan::New<v8::String>(res.sdp, res.sdpLength);
    info.GetReturnValue().Set(responseSdp.ToLocalChecked());
    return;
  }

  info.GetReturnValue().Set(Nan::Null());
}

NAN_METHOD(WuHostWrap::SetUDPWriteFunction) {
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  obj->udpWriteCallback.Reset(Nan::To<v8::Function>(info[0]).ToLocalChecked());
}

NAN_METHOD(WuHostWrap::HandleUDP) {
  if (info.Length() < 2) {
    Nan::ThrowError(kInvalidArgumentCountErr);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  Wu* wu = obj->host.wu;

  auto ctx = info.GetIsolate()->GetCurrentContext();
  auto buf = info[0]->ToObject(ctx).ToLocalChecked();
  char* data = node::Buffer::Data(buf);
  size_t length = node::Buffer::Length(buf);

  auto ipObj = info[1]->ToObject(ctx).ToLocalChecked();
  Nan::Utf8String ipVal(Nan::Get(ipObj, Nan::New("address").ToLocalChecked()).ToLocalChecked());

  struct in_addr address;
  inet_pton(AF_INET, *ipVal, &address);
  uint32_t ip = ntohl(address.s_addr);
  uint32_t port =
      Nan::To<uint32_t>(Nan::Get(ipObj, Nan::New("port").ToLocalChecked()).ToLocalChecked())
          .ToChecked();

  WuAddress remote;
  remote.host = ip;
  remote.port = port;

  WuHandleUDP(wu, &remote, (const uint8_t*) data, length);

  WuHostWrap::Serve(info);
}

NAN_METHOD(WuHostWrap::Serve) {
  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  Wu* wu = obj->host.wu;

  WuEvent evt;
  while (WuUpdate(wu, &evt)) {
    switch (evt.type) {
      case WuEvent_ClientJoin: {
        obj->HandleClientJoin(evt.client);
        break;
      }
      case WuEvent_TextData:
      case WuEvent_BinaryData: {
        obj->HandleContent(&evt);
        break;
      }
      case WuEvent_ClientLeave: {
        obj->HandleClientLeave(evt.client);
        break;
      }
      default:
        break;
    }
  }
}

NAN_METHOD(WuHostWrap::SetClientJoinFunction) {
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  obj->clientJoinCallback.Reset(Nan::To<v8::Function>(info[0]).ToLocalChecked());
}

NAN_METHOD(WuHostWrap::SetClientLeaveFunction) {
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  obj->clientLeaveCallback.Reset(Nan::To<v8::Function>(info[0]).ToLocalChecked());
}

NAN_METHOD(WuHostWrap::SetBinaryDataReceivedFunction) {
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  obj->binaryDataCallback.Reset(Nan::To<v8::Function>(info[0]).ToLocalChecked());
}

NAN_METHOD(WuHostWrap::SetTextDataReceivedFunction) {
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  obj->textDataCallback.Reset(Nan::To<v8::Function>(info[0]).ToLocalChecked());
}

NAN_METHOD(WuHostWrap::RemoveClient) {
  if (info.Length() < 1 || !info[0]->IsNumber()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  uint32_t id = Nan::To<uint32_t>(info[0]).ToChecked();
  obj->RemoveClient(id);
}

NAN_METHOD(WuHostWrap::SendTextData) {
  if (info.Length() < 2 || !info[1]->IsString()) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  uint32_t clientId = Nan::To<uint32_t>(info[0]).ToChecked();

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  if (obj->clients.count(clientId) == 0) {
    return;
  }

  WuClient* client = obj->clients[clientId];
  Nan::Utf8String text(info[1]);

  WuSendText(obj->host.wu, client, *text, text.length());
}

NAN_METHOD(WuHostWrap::SendBinaryData) {
  if (info.Length() < 2) {
    Nan::ThrowError(kInvalidArguments);
    return;
  }

  uint32_t clientId = Nan::To<uint32_t>(info[0]).ToChecked();

  WuHostWrap* obj = Nan::ObjectWrap::Unwrap<WuHostWrap>(info.This());
  if (obj->clients.count(clientId) == 0) {
    return;
  }

  WuClient* client = obj->clients[clientId];

  auto ctx = info.GetIsolate()->GetCurrentContext();
  auto buf = info[1]->ToObject(ctx).ToLocalChecked();
  char* data = node::Buffer::Data(buf);
  size_t length = node::Buffer::Length(buf);

  WuSendBinary(obj->host.wu, client, (uint8_t*) data, length);
}

NAN_MODULE_INIT(Init) { WuHostWrap::Init(target); }
NODE_MODULE(WebUDP, Init);
