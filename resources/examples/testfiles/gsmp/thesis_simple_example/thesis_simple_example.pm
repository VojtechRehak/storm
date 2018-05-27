gsmp

const int maxItem=3;
const distribution Prod_dist = dirac(10);

module Queue

    event Prod_e = Prod_dist;
    event Serve_e = weibull(12,2.0);
    items: [0..maxItem] init 0;

    [production] items < maxItem --Prod_e-> (items'=items+1);
    [consumption] items > 0 --Serve_e-> (items'=items-1);
endmodule

rewards
    items>0 : items;
    [production] true : 1;
endrewards
