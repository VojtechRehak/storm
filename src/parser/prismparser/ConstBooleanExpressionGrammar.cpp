#include "ConstBooleanExpressionGrammar.h"

#include "ConstIntegerExpressionGrammar.h"

namespace storm {
namespace parser {
namespace prism {

	ConstBooleanExpressionGrammar::ConstBooleanExpressionGrammar(std::shared_ptr<VariableState>& state)
		: ConstBooleanExpressionGrammar::base_type(constantBooleanExpression), BaseGrammar(state) {

		constantBooleanExpression %= constantOrExpression;
		constantBooleanExpression.name("constant boolean expression");
		
		constantOrExpression = constantAndExpression[qi::_val = qi::_1] >> *(qi::lit("|") >> constantAndExpression)[qi::_val = phoenix::bind(&BaseGrammar::createOr, this, qi::_val, qi::_1)];
		constantOrExpression.name("constant boolean expression");

		constantAndExpression = constantNotExpression[qi::_val = qi::_1] >> *(qi::lit("&") >> constantNotExpression)[qi::_val = phoenix::bind(&BaseGrammar::createAnd, this, qi::_val, qi::_1)];
		constantAndExpression.name("constant boolean expression");

		constantNotExpression = constantAtomicBooleanExpression[qi::_val = qi::_1] | (qi::lit("!") >> constantAtomicBooleanExpression)[qi::_val = phoenix::bind(&BaseGrammar::createNot, this, qi::_1)];
		constantNotExpression.name("constant boolean expression");

		constantAtomicBooleanExpression %= (constantRelativeExpression | qi::lit("(") >> constantBooleanExpression >> qi::lit(")") | booleanLiteralExpression | booleanConstantExpression);
		constantAtomicBooleanExpression.name("constant boolean expression");

		constantRelativeExpression = (ConstIntegerExpressionGrammar::instance(this->state) >> relations_ >> ConstIntegerExpressionGrammar::instance(this->state))[qi::_val = phoenix::bind(&BaseGrammar::createRelation, this, qi::_1, qi::_2, qi::_3)];
		constantRelativeExpression.name("constant boolean expression");
		
		booleanConstantExpression %= (this->state->booleanConstants_ | booleanLiteralExpression);
		booleanConstantExpression.name("boolean constant or literal");

		booleanLiteralExpression = qi::bool_[qi::_val = phoenix::bind(&BaseGrammar::createBoolLiteral, this, qi::_1)];
		booleanLiteralExpression.name("boolean literal");
	}
}
}
}