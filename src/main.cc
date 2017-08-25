#include <node.h>
#include <node_buffer.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

namespace shm {

	using v8::FunctionCallbackInfo;
	using v8::Isolate;
	using v8::Local;
	using v8::Object;
	using v8::String;
	using v8::Integer;
	using v8::Value;
	using v8::Exception;

	void Method_Ftok(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();

		v8::String::Utf8Value arg0(args[0]);
		v8::String::Utf8Value arg1(args[1]);

		const char *filepath = (args.Length() > 1) ? *arg0 : "";
		const char *project = (args.Length() > 1) ? *arg1 : "";

		key_t key = ftok(filepath, project[0]);

		args.GetReturnValue().Set(Integer::New(isolate, key));
	}

	void Method_Read(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();

		int key = args[0]->Uint32Value();
		int size = args[1]->Uint32Value();

		const int shmid = shmget(key, size, SHM_RDONLY);
		if (shmid == -1) {
			isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "shmget failed")));
		}

		struct shmid_ds shm;
		if (shmctl(shmid, IPC_STAT, &shm)) {
			isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "unable to get shared memory segment information")));
		}

		char *addr = (char *)shmat(shmid, 0, SHM_RDONLY);
		if (addr == (char *) -1) {
			isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "failed to attach")));
		}

		size_t offset = 0;
		size_t length = size;

		args.GetReturnValue().Set(node::Buffer::Copy(isolate, (const char*)((char *)addr + offset), length).ToLocalChecked());
		shmdt(addr);
	}

	void Method_Write(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();

		int key = args[0]->Uint32Value();
		int size = args[1]->Uint32Value();
  		void *dataBuf = node::Buffer::Data(args[2]);
  		size_t dataLen = node::Buffer::Length(args[2]);
		size_t offset = args[3]->Uint32Value();

		const int shmid = shmget(key, size, IPC_CREAT);
		if (shmid == -1) {
			isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "shmget failed")));
		}

		char *addr = (char *)shmat(shmid, 0, IPC_CREAT);
		if (addr == (char *) -1) {
			isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "failed to attach")));
		}

		memcpy((void *)((char *)addr + offset), dataBuf, dataLen);
		shmdt(addr);
	}

	void init(Local<Object> exports) {
		NODE_SET_METHOD(exports, "ftok", Method_Ftok);
		NODE_SET_METHOD(exports, "read", Method_Read);
		NODE_SET_METHOD(exports, "write", Method_Write);
	}

	NODE_MODULE(addon, init)

}