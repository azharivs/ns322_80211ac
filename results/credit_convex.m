% This script calculates the optimal bit allowance allocation for each
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
avgIdle = 1;
global rates;
%rates = [10 20 30 40]'*1e6;
global prRates;
%prRates = ones(4,4)*0.25; %load('bitrates.txt');%column i corresponds to station i, row j to j-th bitrate
%prRates = [0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;
%    0.1 0.25 0.6 0.05;]';
load params1.mat;
%rates = rates*1e6
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
targetDvp = [0.01 0.01 0.01 0.01];
global dMax;
dMax = [1 1 1 1]; %in seconds
global SI;
SI = 0.1; %length of service interval in seconds
global lambda;
lambda = [6 6 6 6]; % data arrival rate (Mbps)
lambdaMat = repmat(lambda,nRates,1);
ub = ones(1,nSta)*0.2; %[1 1 1 1]; %max unused Mbit allowance during an SI
lb = ones(1,nSta)*0.3; %[0 0 0 0]; %max exceeded Mbit allowance during an SI (this can be negative)
ubMat = repmat(ub,nRates,1);
lbMat = repmat(lb,nRates,1);
beta = 0.05*ones(1,nSta); %probability of violating max unused bit allowance
gamma = 0.01*ones(1,nSta);%probability of violating max exceeded bit allowance
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

cvx_begin 
    cvx_solver sedumi
    variables N(nRates,nSta) 
    %maximize (sum(sum(N.*prRates))) % maximize average bits sent per SI
    minimize (sum(sum((N./ratesMat).*prRates)))
    sum(sum((N./ratesMat).*prRates,1),2) <= SI % limit average total TX time during each SI to be less than SI
    sum(N.*prRates,1) == lambda*SI % average data rate constraint
    %sum(prRates.*(1-exp(-max(0,(lambdaMat*SI+ubMat-N)*1e3))),1) >= 1-beta % do not violate max unused bit allowance
    %x == max(0,N-lambdaMat*SI)
    sum(prRates.*max(0,N-lambdaMat*SI),1) <= ub % do not violate average data backlog
    sum(prRates.*min(0,N-lambdaMat*SI),1) >= -lb % do not violate average backlog compensation
    %sum(prRates.*(-lambdaMat*SI-lbMat+N),1) >= 1-gamma % do not violate max exceeded bit allowance
    N >= 0
cvx_end

full(N)
sum(sum(N.*prRates))/SI
sum(sum((N./ratesMat).*prRates,1),2)/SI
sum(prRates.*max(0,N-lambdaMat*SI),1)
sum(prRates.*min(0,N-lambdaMat*SI),1)
NN = N';
%ratesT = rates';

%save T.txt ratesT '-ascii'
%save T.txt TT '-ascii' '-append'

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
