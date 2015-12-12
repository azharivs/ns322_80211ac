function [c, ceq] = constraint(x)
    global pkt;
    global SI;
    global prRates;
    global vectPrRates;
    global ratesMat;
    global const;
    global nSta;
    global nRates;
    global lambda;
    global avgS;
    global rho;
    global avgS2;
    global avgQ;
   
    TT = reshape(x,nRates,nSta);
    %avgS = pkt*SI*sum(prRates./(TT.*ratesMat),1); %average MPDU service time in seconds
    avgS = pkt*SI./sum(prRates.*(TT.*ratesMat),1); %average MPDU service time in seconds
    avgS = min(avgS,9999999);
    avgS2 = ((pkt*SI)^2)*sum(prRates./((TT.^2).*(ratesMat.^2)),1); %second moment of MPDU service time in seconds square
    avgS2 = min(avgS2,9999999);
    %avgS2 = avgS.^2;
    rho = lambda.*avgS; %prob of link queue not empty
    rho = min(0.9999999,rho);
    avgQ = (lambda.^2).*avgS2./(1-rho)/2;

    ceq = [];
    %ceq = [log(1+abs(sum(TT.*prRates.*ratesMat,1) + const.*avgQ./rho))'];
    c = [ (-sum(TT.*prRates.*ratesMat,1) - const.*avgQ./rho)'; % effective capacity constraint
          (avgS.*lambda - 1)'; %queue stability condition
           sum(sum(x.*vectPrRates)) - SI]; %feasibility of time allowances 
       