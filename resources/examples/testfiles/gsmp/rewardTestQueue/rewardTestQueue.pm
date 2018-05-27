// This is a model of G/G/1/n queue.
// The inter-arrival time is specified by Prod_event.
// The service time is specified by Serve_event.

gsmp

const rew = 4;

rewards // nonsensical rewards for testing
	items > 0 : 0.025 + items;
	[consumption] true : rew - (items/rew) ;
	[consumptionFinal] true : rew * rew;
endrewards

rewards // nonsensical second rewards to make sure multiple reward structures still work
	items = 5 : rew * rew - rew;

endrewards

const maxItem=12;


module Queue

	event Prod_event = dirac(10);
	event Serve_event = weibull(12,2.0);

	items: [0..maxItem] init 0;

	[production] items < maxItem --Prod_event-> (items'=items+1);
	[consumption] items > 0 --Serve_event-> (items'=items-1);
	[consumptionFinal] items = 1 --Serve_event-> (items'=items-1);
endmodule
