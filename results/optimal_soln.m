% This script calculates the optimal time allowance allocation for each
% station associate to an AP given steady state channel bitrate
% probabilities. It is based on satisfying the following relationship that
% for each link the average number of aggregated packets during a service
% interval should match
% -SI*log(targetDVP)/dMax*averageQueueSize/probQueueNonEmpty. This is
% derived from assuming an effective capacity exists and applying Little's
% law to average wating time. The following script assumes an M/G/1
% queueing model to calculate average queue length. In addition, no strict
% constraint is set on the total allocatable time allowance per service
% interval. In fact it is implicitly assumed that the sum of time
% allowances allocated to all links can occasionaly violate the length of
% service interval SI. However, we require that the average allocated time
% allowance be less than SI. This is practical since we can occasionally
% handle an enlarged service interval. Should do this for our simulations.
clear all;

global avgIdle;
avgIdle = 0.5;
global rates;
%rates = [10 20 30 40]'*1e6;
global prRates;
%prRates = ones(4,4)*0.25; %load('bitrates.txt');%column i corresponds to station i, row j to j-th bitrate
%prRates = [0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;]';
load params.mat;
rates = rates*1e6
global vectPrRates;
vectPrRates = reshape(prRates,1,size(prRates,1)*size(prRates,2));
global nSta;
nSta = size(prRates,2);
global nRates;
nRates = size(prRates,1);
global ratesMat;
ratesMat = repmat(rates,1,nSta);
global pkt;
pkt = 1500*8; %MPDU size in bits
global targetDvp;
targetDvp = [0.1 0.1 0.1 0.1];
global dMax;
dMax = [1 1 1 1]; %in seconds
global SI;
SI = 0.1; %length of service interval in seconds
global lambda;
lambda = [6.7 6.7 6.7 6.7]*1e6/pkt; % packet (MPDU) arrival rate (pps)
global const;
const = pkt*SI*log(targetDvp)./dMax;
global avgS;
global rho;
global avgS2;
global avgQ;

%this is a non-convex problem!
%this is not even a geometric program!
% cvx_begin 
%     variables T(nRates,nSta) avgS(1,nSta) avgS2(1,nSta) avgQ(1,nSta) rho(1,nSta)
%     minimize (sum(sum(T.*prRates)))
%     avgS == pkt*SI*sum(prRates./(T.*ratesMat),1) %average MPDU service time in seconds
%     avgS2 == ((pkt*SI)^2)*sum(prRates./((T.^2).*(ratesMat.^2)),1) %second moment of MPDU service time in seconds square
%     rho == lambda.*avgS %prob of link queue not empty
%     avgQ == (lambda.^2).*avgS2./(1-rho)/2
%     sum(T.*prRates.*ratesMat,1) + const.*avgQ./rho == zeros(1,nSta) % effective capacity constraint
%     T >= 0
% cvx_end

%Assuming M/M/1 queue model for links
cvx_begin 
    variables T(nRates,nSta)
    minimize (sum(sum(T.*prRates)))
    sum(T.*prRates.*ratesMat,1) - SI*pkt*(lambda-log(targetDvp)./dMax) >= zeros(1,nSta) % effective capacity constraint
    sum(T.*prRates.*ratesMat,1) - lambda*SI*pkt >= zeros(1,nSta) % queue utilization less than one
    -sum(exp(-10000*T).*prRates,1) + 1 - 1/(1+avgIdle) >= zeros(1,nSta) % limits average idle service intervals to avgIdle
    SI - T >= zeros(nRates,nSta)
    %T - pkt./ratesMat >= zeros(nRates,nSta) %send at least one packet for each bitrate
    T >= 0
cvx_end

T
sum(sum(T.*prRates))/SI
TT = T';
ratesT = rates';

save T.txt ratesT '-ascii'
save T.txt TT '-ascii' '-append'

% iterative approach: incomplete
% TT = SI*ones(nRates,nSta)/nRates/nSta;
% while (1==1)
% avgS = pkt*SI*sum(prRates./(TT.*ratesMat),1); %average MPDU service time in seconds
% avgS2 = ((pkt*SI)^2)*sum(prRates./((TT.^2).*(ratesMat.^2)),1); %second moment of MPDU service time in seconds square
% rho = lambda.*avgS; %prob of link queue not empty
% avgQ = (lambda.^2).*avgS2./(1-rho)/2;
% avgQ = avgQ + (rho>=1).*1e13;
% b = -const.*avgQ./rho;
% 
% cvx_begin
%      variables T(nRates,nSta) % avgS(1,nSta) avgS2(1,nSta) avgQ(1,nSta) rho(1,nSta)
%      minimize (sum(sum(T.*prRates))) 
%      sum(T.*prRates.*ratesMat,1) >= b %-const.*avgQ./rho % effective capacity constraint
%      T >= 0
%      %T <= SI
% cvx_end
% 
% a=input('sadfs');
% TT = T;
% end
% 

%Genetic algorithm solution to the optimization problem 
%T0 = ones(1,nRates*nSta)*eps; %SI*ones(1,nRates*nSta)/nRates/nSta;
%objective(T0)
%constraint(T0)
% 
% ObjectiveFunction = @objective;
% nvars = nRates*nSta;    % Number of variables
% LB = zeros(1,size(T0,2))+eps;   % Lower bound
% UB = ones(1,size(T0,2))*SI;  % Upper bound
% ConstraintFunction = @constraint;
% options = gaoptimset('MutationFcn',@mutationadaptfeasible);
% options = gaoptimset(options,'InitialPopulation',T0);
% options = gaoptimset(options,'PlotFcns',{@gaplotbestf,@gaplotmaxconstr},'Display','iter');
% [T,fval] = ga(ObjectiveFunction,nvars,[],[],[],[],LB,UB, ...
%     ConstraintFunction,options)
% reshape(T,nRates,nSta)
