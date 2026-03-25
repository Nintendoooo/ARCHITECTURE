#pragma once

#include <memory>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace taxi_service::auth {

class JwtChecker final
    : public userver::server::handlers::auth::AuthCheckerBase {
 public:
  using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

  explicit JwtChecker(const std::string& secret);

  AuthCheckResult CheckAuth(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  bool SupportsUserAuth() const noexcept override { return true; }

 private:
  std::string secret_;
};

using JwtCheckerPtr = std::shared_ptr<JwtChecker>;

class TaxiJwtAuthComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr auto kName = "taxi-jwt-auth";

  TaxiJwtAuthComponent(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

  JwtCheckerPtr GetChecker() const;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  JwtCheckerPtr checker_;
};

}  // namespace taxi_service::auth
