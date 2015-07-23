close all;
clear all;
nSta = 4;
baseLogName = 'logfiles/log.1';
pattern = {'k-','r-','g-','m-','b-','y-'};
bssPhyMacLogName = sprintf('%s.BssPhyMacStats',baseLogName);
for i=1:nSta
    if (i<16)
        mac = sprintf('00:00:00:00:00:0%X',i);
    else
        mac = sprintf('00:00:00:00:00:%X',i);     
    end
    staQInfoLogName{i} = sprintf('%s.StaQInfo.%s',baseLogName,mac);
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

% Process StaQInfo data
figure;
for i=1:nSta
    data = load(staQInfoLogName{i});
    times = data(:,1);
    avgQPkt = data(:,4);
    avgQWait = data(:,6);
    avgQArrivalMbps = data(:,8);
    avgQDvp = data(:,9);
    clear data;
    subplot(2,2,1);
    plot(times,avgQPkt,pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Avg. Queue Length (pkt)');
    grid on;
    subplot(2,2,2);
    plot(times,avgQWait,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Waiting Time (msec)');
    grid on;
    subplot(2,2,3);
    plot(times,avgQArrivalMbps,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Arrival Rate (Mb/s)');
    grid on;
    subplot(2,2,4);
    plot(times,avgQDvp,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Delay Violation Prob.');
    grid on;
end