#include "storm/storage/prism/EventVariable.h"

namespace storm {
    namespace prism {

        EventVariable::EventVariable(storm::expressions::Variable const& variable, storm::expressions::Expression const& expression, std::string const& filename, uint_fast64_t lineNumber) : Variable(variable, expression, filename, lineNumber) {
            // Intentionally left empty.
        }

        storm::expressions::Expression const& EventVariable::getDistributionExpression() const {
            return this->getInitialValueExpression();
        }

        bool EventVariable::isNonExponential() const {
            return getDistributionExpression().getDistributionType() != expressions::EventDistributionTypes::Exp;
        }

        EventVariable EventVariable::substitute(std::map<storm::expressions::Variable, storm::expressions::Expression> const& substitution) const {
            return EventVariable(this->getExpressionVariable(), this->getDistributionExpression().substitute(substitution), this->getFilename(), this->getLineNumber());
        }

        void EventVariable::createMissingInitialValue() {
        }

        std::ostream& operator<<(std::ostream& stream, EventVariable const& eventVariable) {
            stream << "event " << eventVariable.getName() << " = " << eventVariable.getDistributionExpression() << ";";
            return stream;
        }
    } // namespace prism
} // namespace storm
