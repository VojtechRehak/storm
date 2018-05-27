#include "storm/storage/expressions/Variable.h"
#include "storm/storage/expressions/ExpressionManager.h"

namespace storm {
    namespace expressions {
        Variable::Variable() {
            // Intentionally left empty.
        }
        
        Variable::Variable(std::shared_ptr<ExpressionManager const> const& manager, uint_fast64_t index) : manager(manager), index(index) {
            // Intentionally left empty.
        }
        
        Variable::~Variable() {
            // Intentionally left empty.
        }

        bool Variable::operator==(Variable const& other) const {
            return manager == other.manager && index == other.index;
        }

        bool Variable::operator!=(Variable const& other) const {
            return !(*this == other);
        }
        
        bool Variable::operator<(Variable const& other) const {
            return this->getIndex() < other.getIndex();
        }
        
        storm::expressions::Expression Variable::getExpression() const {
            return storm::expressions::Expression(*this);
        }
        
        uint_fast64_t Variable::getIndex() const {
            return index;
        }
        
        uint_fast64_t Variable::getOffset() const {
            return this->getManager().getOffset(index);
        }
        
        std::string const& Variable::getName() const {
            return this->getManager().getVariableName(index);
        }
        
        Type const& Variable::getType() const {
            return this->getManager().getVariableType(index);
        }
        
        ExpressionManager const& Variable::getManager() const {
            STORM_LOG_ASSERT(manager != nullptr, "Manager is null.");
            return *manager;
        }

        bool Variable::hasBooleanType() const {
            return this->getType().isBooleanType();
        }
        
        bool Variable::hasIntegerType() const {
            return this->getType().isIntegerType();
        }

        bool Variable::hasBitVectorType() const {
            return this->getType().isBitVectorType();
        }
        
        bool Variable::hasRationalType() const {
            return this->getType().isRationalType();
        }

        bool Variable::hasNumericalType() const {
            return this->getType().isNumericalType();
        }

        bool Variable::hasEventDistributionType() const{
            return this->getType().isEventDistributionType();

        }
    }
}
