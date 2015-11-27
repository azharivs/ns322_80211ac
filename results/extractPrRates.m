clear all;
data = load('prRates.txt');
nSta = size(data,1)-1;
k = size(data,2);
tmpRates = data(nSta+1,:);
tmpPrRates = data(1:nSta,:);

rates = sort(unique(tmpRates));
k=0;
for r=rates
    k=k+1;
    prRates(:,k) = sum(tmpPrRates(:,tmpRates == r),2);  
end
rates = rates'
prRates = prRates'
save 'extractedPrRates.mat' prRates rates;