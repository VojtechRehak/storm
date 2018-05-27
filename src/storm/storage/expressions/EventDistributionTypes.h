#ifndef STORM_STORAGE_EXPRESSIONS_EVENTDISTRIBUTIONTYPES_H_
#define STORM_STORAGE_EXPRESSIONS_EVENTDISTRIBUTIONTYPES_H_

namespace storm {
    namespace expressions {
		enum class EventDistributionTypes{
	        Exp,
	        Weibull,
	        Uniform,
	        Dirac,
	        Erlang
        };
    }
}

#endif /* STORM_STORAGE_EXPRESSIONS_EventDistributionTypes_H_ */
