#include "louloulibs.h"

#ifdef BOTAN_FOUND
#include <network/tcp_socket_handler.hpp>
#include <network/credentials_manager.hpp>
#include <logger/logger.hpp>
#include <botan/tls_exceptn.h>
#include <config/config.hpp>

#ifdef USE_DATABASE
# include <database/database.hpp>
#endif

/**
 * TODO find a standard way to find that out.
 */
static const std::vector<std::string> default_cert_files = {
    "/etc/ssl/certs/ca-bundle.crt",
    "/etc/pki/tls/certs/ca-bundle.crt",
    "/etc/ssl/certs/ca-certificates.crt",
    "/etc/ca-certificates/extracted/tls-ca-bundle.pem"
};

Botan::Certificate_Store_In_Memory BasicCredentialsManager::certificate_store;
bool BasicCredentialsManager::certs_loaded = false;

BasicCredentialsManager::BasicCredentialsManager(const TCPSocketHandler* const socket_handler):
    Botan::Credentials_Manager(),
    socket_handler(socket_handler),
    trusted_fingerprint{}
{
  this->load_certs();
}

void BasicCredentialsManager::set_trusted_fingerprint(const std::string& fingerprint)
{
  this->trusted_fingerprint = fingerprint;
}

void BasicCredentialsManager::verify_certificate_chain(const std::string& type,
                                                       const std::string& purported_hostname,
                                                       const std::vector<Botan::X509_Certificate>& certs)
{
  log_debug("Checking remote certificate (", type, ") for hostname ", purported_hostname);
  try
    {
      Botan::Credentials_Manager::verify_certificate_chain(type, purported_hostname, certs);
      log_debug("Certificate is valid");
    }
  catch (const std::exception& tls_exception)
    {
      log_warning("TLS certificate check failed: ", tls_exception.what());
      if (!this->trusted_fingerprint.empty() && !certs.empty() &&
          this->trusted_fingerprint == certs[0].fingerprint() &&
          certs[0].matches_dns_name(purported_hostname))
        // We trust the certificate, based on the trusted fingerprint and
        // the fact that the hostname matches
        return;

      if (this->socket_handler->abort_on_invalid_cert())
        throw;
    }
}

void BasicCredentialsManager::load_certs()
{
  //  Only load the certificates the first time
  if (BasicCredentialsManager::certs_loaded)
    return;
  const std::string conf_path = Config::get("ca_file", "");
  std::vector<std::string> paths;
  if (conf_path.empty())
    paths = default_cert_files;
  else
    paths.push_back(conf_path);
  for (const auto& path: paths)
    {
      try
        {
          Botan::DataSource_Stream bundle(path);
          log_debug("Using ca bundle: ", path);
          while (!bundle.end_of_data() && bundle.check_available(27))
            {
              // TODO: remove this work-around for Botan 1.11.29
              // https://github.com/randombit/botan/issues/438#issuecomment-192866796
              // Note that every certificate that fails to be transcoded into latin-1
              // will be ignored. As a result, some TLS connection may be refused
              // because the certificate is signed by an issuer that was ignored.
              try {
                  const Botan::X509_Certificate cert(bundle);
                  BasicCredentialsManager::certificate_store.add_certificate(cert);
                } catch (const Botan::Decoding_Error& error)
                {
                  continue;
                }
            }
          // Only use the first file that can successfully be read.
          goto success;
        }
      catch (Botan::Stream_IO_Error& e)
        {
          log_debug(e.what());
        }
    }
  //  If we could not open one of the files, print a warning
  log_warning("The CA could not be loaded, TLS negociation will probably fail.");
  success:
  BasicCredentialsManager::certs_loaded = true;
}

std::vector<Botan::Certificate_Store*> BasicCredentialsManager::trusted_certificate_authorities(const std::string&, const std::string&)
{
  return {&this->certificate_store};
}

#endif
