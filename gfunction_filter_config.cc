#include <string>

#include "gfunction_filter.h"

#include "envoy/registry/registry.h"

namespace Solo {
namespace Gfunction {
namespace Configuration {
  
const std::string gFUNCTION_HTTP_FILTER_SCHEMA(R"EOF(
  {
    "$schema": "http://json-schema.org/schema#",
    "type" : "object",
    "properties" : {
      "access_key": {
        "type" : "string"
      },
      "secret_key": {
        "type" : "string"
      },
      "functions": {
        "type" : "object",
        "additionalProperties" : {
          "type" : "object",
          "properties": {
            "func_name" : {"type":"string"},
            "hostname" : {"type":"string"},
            "region" : {"type":"string"},
            "project" : {"type":"string"}
          }
        }
      }
    },
    "required": ["access_key", "secret_key", "functions"],
    "additionalProperties" : false
  }
  )EOF");

class GfunctionFilterConfig : public Envoy::Server::Configuration::NamedHttpFilterConfigFactory {
public:
  Envoy::Server::Configuration::HttpFilterFactoryCb createFilterFactory(const Envoy::Json::Object& json_config, const std::string&,
    Envoy::Server::Configuration::FactoryContext&) override {
    json_config.validateSchema(gFUNCTION_HTTP_FILTER_SCHEMA);
                     
  std::string access_key = json_config.getString("access_key", "");
  std::string secret_key = json_config.getString("secret_key", "");
  const Envoy::Json::ObjectSharedPtr functions_obj = json_config.getObject("functions", false);

  Gfunction::ClusterFunctionMap functions;

  functions_obj->iterate([&functions](const std::string& key, const Envoy::Json::Object& value){
    const std::string cluster_name = key;
    const std::string func_name = value.getString("func_name", "");
    const std::string hostname = value.getString("hostname", "");
    const std::string region = value.getString("region", "");
    const std::string project = value.getString("project", "");
    functions[cluster_name] = Gfunction::Function {
      .func_name_ = func_name,
      .hostname_ = hostname,
      .region_  = region,
      .project_ = project,
    };
    return true;
  });

    return [access_key, secret_key, functions](Envoy::Http::FilterChainFactoryCallbacks& callbacks) -> void {
      auto filter = new Gfunction::GfunctionFilter(std::move(access_key), std::move(secret_key), std::move(functions));
      callbacks.addStreamDecoderFilter(
          Envoy::Http::StreamDecoderFilterSharedPtr{filter});
    };
  }
  std::string name() override { return "Gfunction"; }
};

/**
 * Static registration for this sample filter. @see RegisterFactory.
 */
static Envoy::Registry::RegisterFactory<GfunctionFilterConfig, Envoy::Server::Configuration::NamedHttpFilterConfigFactory>
    register_;

} // Configuration
} // Gfunction
} // Solo
