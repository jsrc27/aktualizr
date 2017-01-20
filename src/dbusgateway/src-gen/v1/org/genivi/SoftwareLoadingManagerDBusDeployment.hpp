/*
* This file was generated by the CommonAPI Generators.
* Used org.genivi.commonapi.dbus 3.1.5.v201601121430.
* Used org.franca.core 0.9.1.201412191134.
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file, You can obtain one at
* http://mozilla.org/MPL/2.0/.
*/

#ifndef V1_ORG_GENIVI_Software_Loading_Manager_DBUS_DEPLOYMENT_HPP_
#define V1_ORG_GENIVI_Software_Loading_Manager_DBUS_DEPLOYMENT_HPP_

#include <v1/org/genivi/SoftwareLoadingManagerDBusDeployment.hpp>        


#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/DBus/DBusDeployment.hpp>
#undef COMMONAPI_INTERNAL_COMPILATION

namespace v1 {
namespace org {
namespace genivi {
namespace SoftwareLoadingManager_ {

// Interface-specific deployment types
typedef CommonAPI::DBus::StructDeployment<
    CommonAPI::DBus::StringDeployment,
    CommonAPI::DBus::StringDeployment,
    CommonAPI::DBus::StringDeployment,
    CommonAPI::EmptyDeployment
> InstalledPackageDeployment_t;

typedef CommonAPI::DBus::StructDeployment<
    CommonAPI::DBus::StringDeployment,
    CommonAPI::DBus::StringDeployment,
    CommonAPI::EmptyDeployment
> InstalledFirmwareDeployment_t;


// Type-specific deployments

// Attribute-specific deployments

// Argument-specific deployments

// Broadcast-specific deployments


} // namespace SoftwareLoadingManager_
} // namespace genivi
} // namespace org
} // namespace v1

#endif // V1_ORG_GENIVI_Software_Loading_Manager_DBUS_DEPLOYMENT_HPP_