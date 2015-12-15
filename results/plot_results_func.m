function out = plot_results_func(baseLogName,nSta)
close all;
%clear all;
out = 0;
dirName = baseLogName(find(baseLogName == '/',1, 'last')+1:end);
matOutputName = sprintf('./plots/%s/%s.mat',dirName,dirName);
dirName = sprintf('./plots/%s',dirName);
mkdir(dirName);
% for time allowance based scheduler and aggregation with PID controller
% and delay of 1 second use:
%baseLogName = 'logfiles/timeallowance_pid.3';
% for edf_RR (deadline) scheduler and deadline based aggregation with a max end to end
% delay of 1 second use:
%baseLogName = 'logfiles/edfrr_deadline_agg.1';
% for edf scheduler and deadline based aggregation with a max end to end
% delay of 1 second use:
%baseLogName = 'logfiles/edf_deadline_agg.1';
pattern = {'k-','r-','g-','m-','b-','y-','c-','ks','bs','rs','k^','r^','g^','m^','kv','rv','gv','mv','bv','yv','cv','k.','r.','g.','m.','b.'};
pattern1 = {'k^','r^','g^','m^','b^','y^','c^'};
pattern2 = {'kv','rv','gv','mv','bv','yv','cv'};
pattern3 = {'k.','r.','g.','m.','b.','y.','c.'};
bssPhyMacLogName = sprintf('%s.BssPhyMacStats',baseLogName);
for i=1:nSta
    if (i<16)
        mac = sprintf('00:00:00:00:00:0%x', i);
    else
        mac = sprintf('00:00:00:00:00:%x', i);
    end
    staQInfoLogName{i} = sprintf('%s.StaQInfo.%s',baseLogName,mac);
    staAggLogName{i} = sprintf('%s.StaAgg.%s',baseLogName,mac);
    staAggCtrlLogName{i} = sprintf('%s.StaAggCtrl.%s',baseLogName,mac);
    staAggPerBitrateTaLogName{i} = sprintf('%s.StaAggPerBitrateTa.%s',baseLogName,mac);
    legendStr{i} = sprintf('STA %d',i);
end

% Process bssPhyMacStats data
data = load(bssPhyMacLogName);
beaconTimes = data(:,1);
avgIdle = data(:,3);
avgBusy = data(:,5);
clear data;
figure;

hold;
plot(beaconTimes,avgIdle,'s-',beaconTimes,avgBusy,'*-');
legend('Average Idle Time Per Beacon','Average Busy Time Per Beacon');
xlabel('Time (seconds)');
ylabel('milli-seconds');
grid;
%saveas(gcf, './plots/AverageIdleTimePerBeacon-AverageBusyTimePerBeacon.fig');
fig1 = sprintf('%s/fig1.fig',dirName);
saveas(gcf, fig1);

% Process StaQInfo and aggregation data
rates = [];
figure;
for i=1:nSta
    %Sta Q Info
    data = load(staQInfoLogName{i});
    timesStaQ{i} = data(:,1);
    qPkt{i} = data(:,2);
    avgQPkt{i} = data(:,4);
    avgQWait{i} = data(:,6);
    avgQArrivalMbps{i} = data(:,8);
    avgQDvp{i} = data(:,9);
    probEmpty{i} = data(:,10);
    clear data;
    subplot(3,3,1);
    plot(timesStaQ{i},avgQPkt{i},pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Avg. Queue Length (pkt)');
    grid on;
    subplot(3,3,9);
    plot(timesStaQ{i},qPkt{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Instantaneous Queue Length (pkt)');
    grid on;
    subplot(3,3,2);
    plot(timesStaQ{i},avgQWait{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Waiting Time (msec)');
    grid on;
    subplot(3,3,3);
    plot(timesStaQ{i},avgQArrivalMbps{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Arrival Rate (Mb/s)');
    ylim([0 10]);
    grid on;
    subplot(3,3,4);
    plot(timesStaQ{i},avgQDvp{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Delay Violation Prob.');
    grid on;
    subplot(3,3,5);
    plot(timesStaQ{i},probEmpty{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Prob. of Empty Queue');
    grid on;
    
    %Aggregation info
    data = load(staAggLogName{i});
    if (~isempty(data))
        timesStaAgg{i} = data(:,1);
        aggPkts{i} = data(:,3);
        dataRate{i} = data(:,4);
        aggTxTime{i} = data(:,5);
        clear data;
        subplot(3,3,6);
        plot(timesStaAgg{i},aggPkts{i},pattern{i});
        hold on;
        xlabel('Time (seconds)');
        ylabel('Size of A-MPDU (packets)');
        grid on;
        subplot(3,3,7);
        plot(timesStaAgg{i},aggTxTime{i},pattern{i});
        hold on;
        xlabel('Time (seconds)');
        ylabel('Tx Time of A-MPDU (msec)');
        grid on;
        subplot(3,3,8);
        plot(timesStaAgg{i},dataRate{i},pattern{i});
        hold on;
        xlabel('Time (seconds)');
        ylabel('Data Rate (Mb/s)');
        grid on;
        
        %extract bitrates and their probabilities
        rates = union(rates,unique(dataRate{i}));
    end
end
%saveas(gcf, './plots/fig2.fig');
fig2 = sprintf('%s/fig2.fig',dirName);
saveas(gcf, fig2);

% Repeat StaQInfo in separate plots
figure;
for i=1:nSta
    %Sta Q Info
    data = load(staQInfoLogName{i});
    times = data(:,1);
    avgQPkt{i} = data(:,4);
    avgQWait{i} = data(:,6);
    avgQArrivalMbps{i} = data(:,8);
    avgQDvp{i} = data(:,9);
    avgServedPkts{i} = data(:,11);
    avgServedBytes{i} = data(:,13);%data(:,12)*1e3;
    clear data;
    subplot(2,3,1);
    plot(times,avgQPkt{i},pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Avg. Queue Length (pkt)');
    grid on;
    subplot(2,3,2);
    plot(times,avgQWait{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Waiting Time (msec)');
    grid on;
    subplot(2,3,3);
    plot(times,avgQArrivalMbps{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Arrival Rate (Mb/s)');
    grid on;
    subplot(2,3,4);
    plot(times,avgQDvp{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Delay Violation Prob.');
    grid on;
    subplot(2,3,5);
    plot(times,avgServedPkts{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Served Pkts /SI');
    grid on;
    subplot(2,3,6);
    plot(times,avgServedBytes{i},pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Served Bytes /SI (KB)');
    grid on;
end

%saveas(gcf, './plots/fig2.fig');
fig3 = sprintf('%s/fig3.fig',dirName);
saveas(gcf, fig3);

% Repeat aggregation info in separate plot
figure;
for i=1:nSta
    %Aggregation info
    data = load(staAggLogName{i});
    if (~isempty(data))
        times = data(:,1);
        aggPkts{i} = data(:,3);
        dataRate{i} = data(:,4);
        aggTxTime{i} = data(:,5);
        clear data;
        subplot(3,3,1);
        plot(times,aggPkts{i},pattern{i});
        hold on;
        legend(legendStr);
        xlabel('Time (seconds)');
        ylabel('Size of A-MPDU (packets)');
        grid on;
        subplot(3,3,2);
        plot(times,aggTxTime{i},pattern{i});
        hold on;
        xlabel('Time (seconds)');
        ylabel('Tx Time of A-MPDU (msec)');
        grid on;
        subplot(3,3,3);
        plot(times,dataRate{i},pattern{i});
        hold on;
        xlabel('Time (seconds)');
        ylabel('Data Rate (Mb/s)');
        grid on;
    end
    %controller info
    data = load(staAggCtrlLogName{i});
    if (~isempty(data))
        timesAggCtrl{i} = data(:,1);
        err{i} = data(:,2);
        ctrlSignal{i} = data(:,3);
        curTimeAllowance{i} = data(:,4);
        newTimeAllowance{i} = data(:,5);
        derivative{i} = data(:,6);
        integral{i} = data(:,7);
        ref{i} = data(:,8);
        errCorr{i} = data(:,10)/100;
        flag = 0;
        if (size(data,2) > 10) %more columns
            flag = 1;
            thrHi{i} = data(:,11); %sva: lines to be commented when no hi/low threshold value exists
            thrLo{i} = data(:,12); %sva: lines to be commented when no hi/low threshold value exists
        end
        clear data;
        subplot(3,3,4);
        plot(timesAggCtrl{i},err{i},pattern{i})
        hold on;
        %flag = 0; %remove line for threshold based PID control
        if (flag)
            plot(timesAggCtrl{i},thrHi{i},pattern3{i}); %sva: lines to be commented when no hi/low threshold value exists
            plot(timesAggCtrl{i},thrLo{i},pattern3{i}); %sva: lines to be commented when no hi/low threshold value exists
            %plot(times,errCorr,pattern1{i});
        end
        xlabel('Time (seconds)');
        ylabel('Error (/H/L) Signals');
        grid on;
        subplot(3,3,5);
        plot(timesAggCtrl{i},ctrlSignal{i},pattern{i})
        hold on;
        xlabel('Time (seconds)');
        ylabel('Control Signal');
        grid on;
        subplot(3,3,6);
        plot(timesAggCtrl{i},newTimeAllowance{i},pattern{i})
        hold on;
        xlabel('Time (seconds)');
        ylabel('New Time Allowance (msec)');
        grid on;
        subplot(3,3,7);
        plot(timesAggCtrl{i},derivative{i},pattern{i})
        hold on;
        xlabel('Time (seconds)');
        ylabel('Derivative Term');
        grid on;
        subplot(3,3,8);
        plot(timesAggCtrl{i},integral{i},pattern{i})
        hold on;
        xlabel('Time (seconds)');
        ylabel('Integral Term');
        grid on;
        subplot(3,3,9);
        plot(timesAggCtrl{i},ref{i},pattern{i})
        hold on;
        xlabel('Time (seconds)');
        ylabel('Reference Signal');
        grid on;
    end %if
    %extract bitrates and their (time averaged) probabilities
    %     for j=1:max(size(rates))
    %         totalTxTime = sum(aggTxTime);
    %         prRates(j,i) = sum(aggTxTime(dataRate == rates(j)))/totalTxTime;
    %         %prRates(j,i) = sum((dataRate == rates(j)))/max(size(dataRate));
    %     end
end

if (exist('prRates') && exist('rates'))
    save('params.mat','prRates','rates');
end
%saveas(gcf, './plots/fig3.fig');
fig4 = sprintf('%s/fig4.fig',dirName);
saveas(gcf, fig4);

% figure;
% %Per Bitrate TimeAlowance Aggregation info
% for i=1:nSta
%     data = load(staAggLogName{i});
%     if (~isempty(data))
%         %remainingTimeAllowance = data(:,2);
%         times = data(:,1);
%         aggPkts = data(:,3);
%         dataRate = data(:,4);
%         aggTxTime = data(:,5);
%         clear data;
%         busy(i)=0;
%         for j=1:max(size(rates))
%             usedAllowance(j,:)=cumsum(aggTxTime.*(dataRate == rates(j)))';
%             ratesActualProb(j,i) = usedAllowance(j,end)/sum(aggTxTime);
%             busy(i) = busy(i)+usedAllowance(j,end);
%             %rta = remainingTimeAllowance(indexes)./times(indexes)/1e3;
%             %semilogy(times(indexes),rta,pattern{j});
%             subplot(2,2,i);
%             semilogy(times,usedAllowance(j,:),pattern{j});
%             hold on;
%             legend('6.5Mbps','13Mbps','26Mbps','39Mbps','52Mbps','58Mbps','65Mbps','78Mbps','104Mbps','117Mbps','130Mbps');
%         end
%         clear usedAllowance;
%         xlabel('Time (seconds)');
%         ylabel('Used Time Allowance (msec)');
%         grid on;
%     end
% end
%
% ratesActualProb
% busy

save(matOutputName);
