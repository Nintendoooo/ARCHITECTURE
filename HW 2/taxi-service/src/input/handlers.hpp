#pragma once

#include <memory>
#include <string>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../auth/jwt_auth_checker.hpp"
#include "../auth/jwt_auth_factory.hpp"
#include "../common/database.hpp"
#include "../common/models.hpp"

namespace taxi_service::input {

class CreateUserHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-user";

  CreateUserHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

class LoginHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-login";

  LoginHandler(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;

  std::string IssueToken(int64_t user_id, const std::string& role) const;
};

class FindUsersHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-find-users";

  FindUsersHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

class RegisterDriverHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-register-driver";

  RegisterDriverHandler(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<Database> db_;
};

void AppendInputHandlers(userver::components::ComponentList& component_list);

}  // namespace taxi_service::input
