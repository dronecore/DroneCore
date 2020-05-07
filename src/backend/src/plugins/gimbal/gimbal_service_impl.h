// WARNING: THIS FILE IS AUTOGENERATED! As such, it should not be edited.
// Edits need to be made to the proto files
// (see https://github.com/mavlink/MAVSDK-Proto/blob/master/protos/gimbal/gimbal.proto)

#include "gimbal/gimbal.grpc.pb.h"
#include "plugins/gimbal/gimbal.h"

#include "log.h"
#include <atomic>
#include <cmath>
#include <future>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

namespace mavsdk {
namespace backend {

template<typename Gimbal = Gimbal>
class GimbalServiceImpl final : public rpc::gimbal::GimbalService::Service {
public:
    GimbalServiceImpl(Gimbal& gimbal) : _gimbal(gimbal) {}

    template<typename ResponseType>
    void fillResponseWithResult(ResponseType* response, mavsdk::Gimbal::Result& result) const
    {
        auto rpc_result = translateToRpcResult(result);

        auto* rpc_gimbal_result = new rpc::gimbal::GimbalResult();
        rpc_gimbal_result->set_result(rpc_result);
        std::stringstream ss;
        ss << result;
        rpc_gimbal_result->set_result_str(ss.str());

        response->set_allocated_gimbal_result(rpc_gimbal_result);
    }

    static rpc::gimbal::GimbalMode
    translateToRpcGimbalMode(const mavsdk::Gimbal::GimbalMode& gimbal_mode)
    {
        switch (gimbal_mode) {
            default:
                LogErr() << "Unknown gimbal_mode enum value: " << static_cast<int>(gimbal_mode);
            // FALLTHROUGH
            case mavsdk::Gimbal::GimbalMode::YawFollow:
                return rpc::gimbal::GIMBAL_MODE_YAW_FOLLOW;
            case mavsdk::Gimbal::GimbalMode::YawLock:
                return rpc::gimbal::GIMBAL_MODE_YAW_LOCK;
        }
    }

    static mavsdk::Gimbal::GimbalMode
    translateFromRpcGimbalMode(const rpc::gimbal::GimbalMode gimbal_mode)
    {
        switch (gimbal_mode) {
            default:
                LogErr() << "Unknown gimbal_mode enum value: " << static_cast<int>(gimbal_mode);
            // FALLTHROUGH
            case rpc::gimbal::GIMBAL_MODE_YAW_FOLLOW:
                return mavsdk::Gimbal::GimbalMode::YawFollow;
            case rpc::gimbal::GIMBAL_MODE_YAW_LOCK:
                return mavsdk::Gimbal::GimbalMode::YawLock;
        }
    }

    static rpc::gimbal::GimbalResult::Result
    translateToRpcResult(const mavsdk::Gimbal::Result& result)
    {
        switch (result) {
            default:
                LogErr() << "Unknown result enum value: " << static_cast<int>(result);
            // FALLTHROUGH
            case mavsdk::Gimbal::Result::Unknown:
                return rpc::gimbal::GimbalResult_Result_RESULT_UNKNOWN;
            case mavsdk::Gimbal::Result::Success:
                return rpc::gimbal::GimbalResult_Result_RESULT_SUCCESS;
            case mavsdk::Gimbal::Result::Error:
                return rpc::gimbal::GimbalResult_Result_RESULT_ERROR;
            case mavsdk::Gimbal::Result::Timeout:
                return rpc::gimbal::GimbalResult_Result_RESULT_TIMEOUT;
        }
    }

    static mavsdk::Gimbal::Result
    translateFromRpcResult(const rpc::gimbal::GimbalResult::Result result)
    {
        switch (result) {
            default:
                LogErr() << "Unknown result enum value: " << static_cast<int>(result);
            // FALLTHROUGH
            case rpc::gimbal::GimbalResult_Result_RESULT_UNKNOWN:
                return mavsdk::Gimbal::Result::Unknown;
            case rpc::gimbal::GimbalResult_Result_RESULT_SUCCESS:
                return mavsdk::Gimbal::Result::Success;
            case rpc::gimbal::GimbalResult_Result_RESULT_ERROR:
                return mavsdk::Gimbal::Result::Error;
            case rpc::gimbal::GimbalResult_Result_RESULT_TIMEOUT:
                return mavsdk::Gimbal::Result::Timeout;
        }
    }

    grpc::Status SetPitchAndYaw(
        grpc::ServerContext* /* context */,
        const rpc::gimbal::SetPitchAndYawRequest* request,
        rpc::gimbal::SetPitchAndYawResponse* response) override
    {
        if (request == nullptr) {
            LogWarn() << "SetPitchAndYaw sent with a null request! Ignoring...";
            return grpc::Status::OK;
        }

        auto result = _gimbal.set_pitch_and_yaw(request->pitch_deg(), request->yaw_deg());

        if (response != nullptr) {
            fillResponseWithResult(response, result);
        }

        return grpc::Status::OK;
    }

    grpc::Status SetMode(
        grpc::ServerContext* /* context */,
        const rpc::gimbal::SetModeRequest* request,
        rpc::gimbal::SetModeResponse* response) override
    {
        if (request == nullptr) {
            LogWarn() << "SetMode sent with a null request! Ignoring...";
            return grpc::Status::OK;
        }

        auto result = _gimbal.set_mode(translateFromRpcGimbalMode(request->gimbal_mode()));

        if (response != nullptr) {
            fillResponseWithResult(response, result);
        }

        return grpc::Status::OK;
    }

    grpc::Status SetRoiLocation(
        grpc::ServerContext* /* context */,
        const rpc::gimbal::SetRoiLocationRequest* request,
        rpc::gimbal::SetRoiLocationResponse* response) override
    {
        if (request == nullptr) {
            LogWarn() << "SetRoiLocation sent with a null request! Ignoring...";
            return grpc::Status::OK;
        }

        auto result = _gimbal.set_roi_location(
            request->latitude_deg(), request->longitude_deg(), request->altitude_m());

        if (response != nullptr) {
            fillResponseWithResult(response, result);
        }

        return grpc::Status::OK;
    }

    void stop()
    {
        _stopped.store(true);
        for (auto& prom : _stream_stop_promises) {
            if (auto handle = prom.lock()) {
                handle->set_value();
            }
        }
    }

private:
    void register_stream_stop_promise(std::weak_ptr<std::promise<void>> prom)
    {
        // If we have already stopped, set promise immediately and don't add it to list.
        if (_stopped.load()) {
            if (auto handle = prom.lock()) {
                handle->set_value();
            }
        } else {
            _stream_stop_promises.push_back(prom);
        }
    }

    void unregister_stream_stop_promise(std::shared_ptr<std::promise<void>> prom)
    {
        for (auto it = _stream_stop_promises.begin(); it != _stream_stop_promises.end();
             /* ++it */) {
            if (it->lock() == prom) {
                it = _stream_stop_promises.erase(it);
            } else {
                ++it;
            }
        }
    }

    Gimbal& _gimbal;
    std::atomic<bool> _stopped{false};
    std::vector<std::weak_ptr<std::promise<void>>> _stream_stop_promises{};
};

} // namespace backend
} // namespace mavsdk