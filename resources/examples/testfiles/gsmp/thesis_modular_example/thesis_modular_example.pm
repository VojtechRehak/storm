gsmp

const K = 2;
const int N = 2;
const double CUSTOMER_RETURN_PROB = 0.6;
const double THINKING_RATE = 0.9;

module Producer
    thinking_customers: [0..K] init K; // customers who are still deciding
    
    [think] (thinking_customers > 0)-> (THINKING_RATE *
      thinking_customers) : (thinking_customers' = thinking_customers - 1);
    [serve] true --slave-> (CUSTOMER_RETURN_PROB) : 
    (thinking_customers' = thinking_customers + 1) + (1-CUSTOMER_RETURN_PROB) : (thinking_customers' = thinking_customers);
    [serve_last] true --slave-> CUSTOMER_RETURN_PROB : 
    (thinking_customers' = thinking_customers + 1) + (1-CUSTOMER_RETURN_PROB) : (thinking_customers' = thinking_customers);
endmodule

module Server
    event Serve_e = dirac(1.0);
    serving: [0..1] init 0; // 0 - currently not serving, 1 - serving
    
    [serve] true --Serve_e-> (serving' = 1);
    [serve_last] true --Serve_e-> (serving' = 0);
endmodule

module Queue
    customers: [0..N] init 0; // number of customers in the queue
    
    [think] (customers < N) --slave-> (customers'=customers+1);
    [serve] (customers > 1) --slave-> (customers' = customers - 1);
    [serve_last] (customers = 1) --slave-> (customers' = customers - 1);
endmodule
