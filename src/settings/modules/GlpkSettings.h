#ifndef STORM_SETTINGS_MODULES_GLPKSETTINGS_H_
#define STORM_SETTINGS_MODULES_GLPKSETTINGS_H_

#include "src/settings/modules/ModuleSettings.h"

namespace storm {
    namespace settings {
        namespace modules {
            
            /*!
             * This class represents the settings for glpk.
             */
            class GlpkSettings : public ModuleSettings {
            public:
                /*!
                 * Creates a new set of glpk settings that is managed by the given manager.
                 *
                 * @param settingsManager The responsible manager.
                 */
                GlpkSettings(storm::settings::SettingsManager& settingsManager);
                
                /*!
                 * Retrieves whether the output option was set.
                 *
                 * @return True iff the output option was set.
                 */
                bool isOutputSet() const;
                
                /*!
                 * Retrieves the integer tolerance to be used.
                 *
                 * @return The integer tolerance to be used.
                 */
                double getIntegerTolerance() const;
                
                // The name of the module.
                static const std::string moduleName;
                
            private:
                // Define the string names of the options as constants.
                static const std::string integerToleranceOption;
                static const std::string outputOptionName;
            };
            
        } // namespace modules
    } // namespace settings
} // namespace storm

#endif /* STORM_SETTINGS_MODULES_GLPKSETTINGS_H_ */
