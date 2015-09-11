clear all;
close all;
curDir = pwd;
kk=0;
util = figure;
ta = figure;
pattern0 = {'ks-','ko-','kx-','k*-','kd-','k^-','kv-'};

for nsta=[1 6 8]
    ii=0;
    kk=kk+1;
    lgnd{kk} = sprintf('%d STA(s)',nsta);
    for dmax=[1 2 3 5 7 10 19]
        ii = ii+1;
        dvp = 0.01;
        str = sprintf('plots/pid_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp);
        cd(str);
        base = str(find(str == '/',1, 'last')+1:end);
        matFile = sprintf('%s.mat',base)
        load(matFile);%load all variables
        cd(curDir);
        d_max(ii) = dmax;
        util_pid(kk,ii) = mean(avgBusy)/100;
        avg_ta(kk,ii) = mean(newTimeAllowance{1}); %uses maximum of all stations
        for jj=2:nsta
            avg_ta(kk,ii) = avg_ta(kk,ii)+mean(newTimeAllowance{jj});
        end
        avg_ta(kk,ii) = avg_ta(kk,ii)/nsta;
    end
    
    figure(util);
    plot(d_max,util_pid(kk,:),pattern0{kk},'MarkerSize',10,'LineWidth',2);
    grid;
    ylim([0 1]);
    hold on;
    xlabel('Delay Bound (Sec)','fontsize',16);
    ylabel('Channel Utilization','fontsize',16);
    set(gca,'fontsize',16);
    
    figure(ta);
    plot(d_max,avg_ta(kk,:),pattern0{kk},'MarkerSize',10,'LineWidth',2);
    grid;
    ylim([0 16]);
    hold on;
    xlabel('Delay Bound (Sec)','fontsize',16);
    ylabel('Average Time Allowance per STA per BI (msec)','fontsize',16);
    set(gca,'fontsize',16);
        
end
figure(util);
legend(lgnd);

figure(ta);
legend(lgnd);