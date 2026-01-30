// Copyright (c) 2025 Elias Bachaalany
// SPDX-License-Identifier: MIT

#include <opencode/session.hpp>
#include <opencode/client.hpp>

namespace opencode
{

Session::Session(Client* client, SessionInfo info)
    : client_(client), info_(std::move(info))
{
}

Session::~Session() = default;

Session::Session(Session&&) noexcept = default;
Session& Session::operator=(Session&&) noexcept = default;

MessageWithParts Session::send(const std::string& prompt)
{
    const auto& opts = client_->options();
    return client_->send_message(
        info_.id,
        prompt,
        opts.default_provider.value_or(""),
        opts.default_model.value_or("")
    );
}

MessageWithParts Session::send(
    const std::string& prompt,
    const std::string& provider_id,
    const std::string& model_id)
{
    return client_->send_message(info_.id, prompt, provider_id, model_id);
}

void Session::send_streaming(const std::string& prompt, StreamOptions options)
{
    const auto& opts = client_->options();
    client_->send_message_streaming(
        info_.id,
        prompt,
        opts.default_provider.value_or(""),
        opts.default_model.value_or(""),
        std::move(options)
    );
}

void Session::send_streaming(
    const std::string& prompt,
    const std::string& provider_id,
    const std::string& model_id,
    StreamOptions options)
{
    client_->send_message_streaming(
        info_.id,
        prompt,
        provider_id,
        model_id,
        std::move(options)
    );
}

std::vector<MessageWithParts> Session::messages(std::optional<int> limit)
{
    return client_->get_messages(info_.id, limit);
}

bool Session::abort()
{
    return client_->abort_session(info_.id);
}

bool Session::init(const std::string& provider_id, const std::string& model_id)
{
    return client_->init_session(info_.id, provider_id, model_id);
}

std::string Session::summarize(const std::string& provider_id, const std::string& model_id)
{
    return client_->summarize_session(info_.id, provider_id, model_id);
}

SessionInfo Session::revert(const std::string& message_id, const std::string& part_id)
{
    info_ = client_->revert_message(info_.id, message_id, part_id);
    return info_;
}

SessionInfo Session::unrevert()
{
    info_ = client_->unrevert_session(info_.id);
    return info_;
}

SessionInfo Session::share()
{
    info_ = client_->share_session(info_.id);
    return info_;
}

SessionInfo Session::unshare()
{
    info_ = client_->unshare_session(info_.id);
    return info_;
}

bool Session::destroy()
{
    return client_->delete_session(info_.id);
}

} // namespace opencode
