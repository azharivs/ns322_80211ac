function generate_plots(str)
close all;
curDir = pwd;
cd(str); 
base = str(find(str == '/',1, 'last')+1:end);
matFile = sprintf('%s.mat',base);
load(matFile);%load all variables

if (exist('beaconTimes'))
    figure;
    hold;
    plot(beaconTimes,avgIdle,'s-',beaconTimes,avgBusy,'*-');
    legend('Average Idle Time Per Beacon','Average Busy Time Per Beacon','fontsize',24);
    xlabel('Time (seconds)','fontsize',24);
    ylabel('milli-seconds','fontsize',24);
    set(gca,'FontSize',24);
    grid;
    figname = sprintf('%s.busy',base);
    figname(figname=='.') = '_';
    saveas(gcf,figname,'fig');
    saveas(gcf,figname,'eps');
    saveas(gcf,figname,'png');
end

if (exist('timesStaQ'))
    figure;
    hold on;
    for i=1:nSta
        plot(timesStaQ{i},avgQPkt{i},pattern{i});
        legend(legendStr);
        xlabel('Time (seconds)','fontsize',24);
        ylabel('Avg. Queue Length (pkt)','fontsize',24);
        grid on;
        set(gca,'FontSize',24);
        figname = sprintf('%s.avgQ',base);
        figname(figname=='.') = '_';
        saveas(gcf,figname,'fig');
        saveas(gcf,figname,'eps');
        saveas(gcf,figname,'png');
    end
end

if (exist('timesStaQ'))
    figure;
    hold on;
    for i=1:nSta
        plot(timesStaQ{i},avgQWait{i},pattern{i});
        legend(legendStr);
        xlabel('Time (seconds)','fontsize',24);
        ylabel('Avg. Waiting Time (msec)','fontsize',24);
        grid on;
        set(gca,'FontSize',24);
        figname = sprintf('%s.avgW',base);
        figname(figname=='.') = '_';
        saveas(gcf,figname,'fig');
        saveas(gcf,figname,'eps');
        saveas(gcf,figname,'png');
    end
end

if (exist('timesStaQ'))
    figure;
    hold on;
    for i=1:nSta
        avgQDvp{i} = filter(ones(1,1000)/1000,1,avgQDvp{i});
        semilogy(timesStaQ{i},avgQDvp{i},pattern{i});
        legend(legendStr);
        xlabel('Time (seconds)','fontsize',24);
        ylabel('Delay Violation Prob.','fontsize',24);
        ylim([0 1.001]);
        grid on;
        set(gca,'FontSize',24);
        set(gca,'YScale','log');
        figname = sprintf('%s.dvp',base);
        figname(figname=='.') = '_';
        saveas(gcf,figname,'fig');
        saveas(gcf,figname,'eps');
        saveas(gcf,figname,'png');
    end
end

if (exist('timesAggCtrl'))
    figure;
    hold on;
    for i=1:nSta
        plot(timesAggCtrl{i},newTimeAllowance{i},pattern{i});
        legend(legendStr);
        xlabel('Time (seconds)','fontsize',24);
        ylabel('Time Allowance (msec)','fontsize',24);
        grid on;
        set(gca,'FontSize',24);
        figname = sprintf('%s.ta',base);
        figname(figname=='.') = '_';
        saveas(gcf,figname,'fig');
        saveas(gcf,figname,'eps');
        saveas(gcf,figname,'png');
        i
        mean(newTimeAllowance{i})
    end
end


cd(curDir);