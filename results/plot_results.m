close all;
clear all;
nSta = 4;
baseLogName = 'logfiles/log_mimo_channel_ta_noctrl.1';
pattern = {'k-','r-','g-','m-','b-','y-'};
bssPhyMacLogName = sprintf('%s.BssPhyMacStats',baseLogName);
for i=1:nSta
    if (i<16)
        mac = sprintf('00:00:00:00:00:0%X',i);
    else
        mac = sprintf('00:00:00:00:00:%X',i);     
    end
    staQInfoLogName{i} = sprintf('%s.StaQInfo.%s',baseLogName,mac);
    staAggLogName{i} = sprintf('%s.StaAgg.%s',baseLogName,mac);
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

% Process StaQInfo and aggregation data
figure;
for i=1:nSta
    %Sta Q Info
    data = load(staQInfoLogName{i});
    times = data(:,1);
    qPkt = data(:,2);
    avgQPkt = data(:,4);
    avgQWait = data(:,6);
    avgQArrivalMbps = data(:,8);
    avgQDvp = data(:,9);
    probEmpty = data(:,10);
    clear data;
    subplot(3,3,1);
    plot(times,avgQPkt,pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Avg. Queue Length (pkt)');
    grid on;
    subplot(3,3,9);
    plot(times,qPkt,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Instantaneous Queue Length (pkt)');
    grid on;
    subplot(3,3,2);
    plot(times,avgQWait,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Waiting Time (msec)');
    grid on;
    subplot(3,3,3);
    plot(times,avgQArrivalMbps,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Arrival Rate (Mb/s)');
    grid on;
    subplot(3,3,4);
    plot(times,avgQDvp,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Delay Violation Prob.');
    grid on;
    subplot(3,3,5);
    plot(times,probEmpty,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Prob. of Empty Queue');
    grid on;
    
    %Aggregation info
    data = load(staAggLogName{i});
    times = data(:,1);
    aggPkts = data(:,3);
    dataRate = data(:,4);
    aggTxTime = data(:,5);
    clear data;
    subplot(3,3,6);
    plot(times,aggPkts,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Size of A-MPDU (packets)');
    grid on;
    subplot(3,3,7);
    plot(times,aggTxTime,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Tx Time of A-MPDU (msec)');
    grid on;
    subplot(3,3,8);
    plot(times,dataRate,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Data Rate (Mb/s)');
    grid on;    
end

% Repeat StaQInfo in separate plots
figure;
for i=1:nSta
    %Sta Q Info
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



% Repeat aggregation info in separate plot
figure;
for i=1:nSta
    %Aggregation info
    data = load(staAggLogName{i});
    times = data(:,1);
    aggPkts = data(:,3);
    dataRate = data(:,4);
    aggTxTime = data(:,5);
    clear data;
    subplot(2,2,1);
    plot(times,aggPkts,pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Size of A-MPDU (packets)');
    grid on;
    subplot(2,2,2);
    plot(times,aggTxTime,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Tx Time of A-MPDU (msec)');
    grid on;
    subplot(2,2,3);
    plot(times,dataRate,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Data Rate (Mb/s)');
    grid on;
    subplot(2,2,4);
    plot(times,aggPkts*1472*8./dataRate/1000 - aggTxTime,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('Computed Tx Time - Logged Tx Time (msec)');
    grid on;
end


