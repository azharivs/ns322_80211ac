clear all;
close all;
curDir = pwd;
kk=0;
util = figure;
ta = figure;
pattern0 = {'ks-','ko-','kx-','k*-','kd-','k^-','kv-'};
font_size = 24;
marker_size = 14;

for nsta=[1 2 4 6 8]
    ii=0;
    kk=kk+1;
    lgnd{kk} = sprintf('%d STA(s)',nsta);
    for dvp=[0.01 0.02 0.05 0.10]
        ii = ii+1;
        dmax = 5;
        str = sprintf('plots/pid_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp);
        cd(str);
        base = str(find(str == '/',1, 'last')+1:end);
        matFile = sprintf('%s.mat',base)
        load(matFile);%load all variables
        cd(curDir);
        dvProb(ii) = dvp;
        util_pid(kk,ii) = mean(avgBusy)/100;
        avg_ta(kk,ii) = mean(newTimeAllowance{1}); %uses maximum of all stations
        for jj=2:nsta
            avg_ta(kk,ii) = avg_ta(kk,ii)+mean(newTimeAllowance{jj});
        end
        avg_ta(kk,ii) = avg_ta(kk,ii)/nsta;
    end
    
    figure(util);
    plot(dvProb,util_pid(kk,:),pattern0{kk},'MarkerSize',marker_size,'LineWidth',2);
    grid;
    ylim([0 1]);
    hold on;
    xlabel('Delay Violation Probability','fontsize',font_size);
    ylabel('Channel Utilization','fontsize',font_size);
    set(gca,'fontsize',font_size);
    
    figure(ta);
    plot(dvProb,avg_ta(kk,:),pattern0{kk},'MarkerSize',marker_size,'LineWidth',2);
    grid;
    ylim([0 16]);
    hold on;
    xlabel('Delay Violation Probability','fontsize',font_size);
    ylabel('Average Time Allowance per STA per BI (msec)','fontsize',font_size);
    set(gca,'fontsize',font_size);
        
end
figure(util);
legend(lgnd);

figure(ta);
legend(lgnd);