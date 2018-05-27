#ifndef STORM_STORAGE_PRISM_EventVariable_H_
#define STORM_STORAGE_PRISM_EventVariable_H_

#include "storm/storage/prism/Variable.h"
#include <iostream>

namespace storm {
    namespace prism {
        class EventVariable : public Variable {
        public:
        	
            EventVariable(storm::expressions::Variable const& variable, storm::expressions::Expression const& expression, std::string const& filename = "", uint_fast64_t lineNumber = 0);

			EventVariable() = default;
            EventVariable(EventVariable const& other) = default;
            EventVariable& operator=(EventVariable const& other)= default;
            EventVariable(EventVariable&& other) = default;
            EventVariable& operator=(EventVariable&& other) = default;

            /*!
             * Retrieves the name of the distribution that is associated with this EventVariable.
             *
             * @return The distribution that is associated with this EventVariable.
             */
            storm::expressions::Expression const& getDistributionExpression() const;

            bool isNonExponential() const;

            /*!
             * Substitutes all identifiers in the event variable according to the given map.
             *
             * @param substitution The substitution to perform.
             * @return The resulting event variable.
             */
            EventVariable substitute(std::map<storm::expressions::Variable, storm::expressions::Expression> const& substutution) const;

            virtual void createMissingInitialValue() override;

            /*!
             * Retrieves the return type of the EventVariable, i.e., the return-type of the defining expression.
             *
             * @return The return type of the EventVariable.
             */
            friend std::ostream& operator<<(std::ostream& stream, EventVariable const& EventVariable);
		};
	}
}

#endif /* STORM_STORAGE_PRISM_EventVariable_H_	*/
