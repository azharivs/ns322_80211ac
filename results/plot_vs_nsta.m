clear all;
close all;
curDir = pwd;
ii=0;
for nsta=[1:10]
    nsta
    ii = ii+1;
    dmax = 5;
    dvp = 0.01;
    str = sprintf('plots/pid_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp)
    cd(str);
    base = str(find(str == '/',1, 'last')+1:end)
    matFile = sprintf('%s.mat',base)
    load(matFile);%load all variables
    cd(curDir);
    n_sta(ii) = ii;
    util_pid(ii) = mean(avgBusy)/100;
    avg_delay(ii) = mean(avgQWait{1}); %Caution! uses first station as sample for average queue waiting time
    avg_dvp(ii) = mean(avgQDvp{1}); %uses maximum of all stations
    for jj=2:nsta
        avg_dvp(ii) = max(avg_dvp(ii),mean(avgQDvp{jj})); 
    end
    avg_dvp(ii) = max(avg_dvp(ii),eps);
end

figure;
plot(n_sta,util_pid,'ks-','MarkerSize',10,'LineWidth',2);
grid;
ylim([0 1]);
hold on;
xlabel('Number of Stations','fontsize',16);
ylabel('Channel Utilization','fontsize',16);
set(gca,'fontsize',16);

figure;
plot(n_sta,avg_delay/1000,'ks-','MarkerSize',10,'LineWidth',2);
grid;
hold on;
xlabel('Number of Stations','fontsize',16);
ylabel('Average End to End Delay (Seconds)','fontsize',16);
set(gca,'fontsize',16);

figure;
semilogy(n_sta,avg_dvp,'ks-','MarkerSize',10,'LineWidth',2);
grid;
hold on;
ylim([0.001 1]);
xlim([1 10]);
line([1 10],[dvp dvp],'LineWidth',2);
xlabel('Number of Stations','fontsize',16);
ylabel('Delay Violation Probability','fontsize',16);
set(gca,'fontsize',16);
