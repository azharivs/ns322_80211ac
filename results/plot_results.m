close all;
clear all;
nSta = 4;
baseLogName = 'logfiles/log_mimo_channel_ta_pid.atan.1';
pattern = {'k-','r-','g-','m-','b-','y-','c-','ks','bs','rs'};
pattern1 = {'k^','r^','g^','m^','b^','y^','c^'};
pattern2 = {'kv','rv','gv','mv','bv','yv','cv'};
pattern3 = {'k.','r.','g.','m.','b.','y.','c.'};
bssPhyMacLogName = sprintf('%s.BssPhyMacStats',baseLogName);
for i=1:nSta
    if (i<16)
        mac = sprintf('00:00:00:00:00:0%X',i);
    else
        mac = sprintf('00:00:00:00:00:%X',i);     
    end
    staQInfoLogName{i} = sprintf('%s.StaQInfo.%s',baseLogName,mac);
    staAggLogName{i} = sprintf('%s.StaAgg.%s',baseLogName,mac);
    staAggCtrlLogName{i} = sprintf('%s.StaAggCtrl.%s',baseLogName,mac);
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
rates = [];
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
    
    %extract bitrates and their probabilities 
    rates = union(rates,unique(dataRate));
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
    avgServedPkts = data(:,11);
    avgServedBytes = data(:,12)*1e3;
    clear data;
    subplot(2,3,1);
    plot(times,avgQPkt,pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Avg. Queue Length (pkt)');
    grid on;
    subplot(2,3,2);
    plot(times,avgQWait,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Waiting Time (msec)');
    grid on;
    subplot(2,3,3);
    plot(times,avgQArrivalMbps,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Arrival Rate (Mb/s)');
    grid on;
    subplot(2,3,4);
    plot(times,avgQDvp,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Delay Violation Prob.');
    grid on;
    subplot(2,3,5);
    plot(times,avgServedPkts,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Served Pkts /SI');
    grid on;
    subplot(2,3,6);
    plot(times,avgServedBytes,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Avg. Served Bytes /SI (KB)');
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
    subplot(3,3,1);
    plot(times,aggPkts,pattern{i});
    hold on;
    legend(legendStr);
    xlabel('Time (seconds)');
    ylabel('Size of A-MPDU (packets)');
    grid on;
    subplot(3,3,2);
    plot(times,aggTxTime,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Tx Time of A-MPDU (msec)');
    grid on;
    subplot(3,3,3);
    plot(times,dataRate,pattern{i});
    hold on;
    xlabel('Time (seconds)');
    ylabel('Data Rate (Mb/s)');
    grid on;
    %controller info
    data = load(staAggCtrlLogName{i});
    times = data(:,1);
    err = data(:,2);
    ctrlSignal = data(:,3);
    curTimeAllowance = data(:,4);
    newTimeAllowance = data(:,5);
    derivative = data(:,6);
    integral = data(:,7);
    ref = data(:,8);
    errCorr = data(:,10)/100;
    thrHi = data(:,11); %sva: lines to be commented when no hi/low threshold value exists
    thrLo = data(:,12); %sva: lines to be commented when no hi/low threshold value exists
    clear data;
    subplot(3,3,4);
    plot(times,err,pattern{i})
    hold on;
    plot(times,thrHi,pattern3{i}); %sva: lines to be commented when no hi/low threshold value exists
    plot(times,thrLo,pattern3{i}); %sva: lines to be commented when no hi/low threshold value exists
    plot(times,errCorr,pattern1{i});
    xlabel('Time (seconds)');
    ylabel('Error (/H/L) Signals');
    grid on;
    subplot(3,3,5);
    plot(times,ctrlSignal,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('Control Signal');
    grid on;
    subplot(3,3,6);
    plot(times,newTimeAllowance,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('New Time Allowance (msec)');
    grid on;
    subplot(3,3,7);
    plot(times,derivative,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('Derivative Term');
    grid on;
    subplot(3,3,8);
    plot(times,integral,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('Integral Term');
    grid on;
    subplot(3,3,9);
    plot(times,ref,pattern{i})
    hold on;
    xlabel('Time (seconds)');
    ylabel('Reference Signal');
    grid on;

    %extract bitrates and their probabilities 
    for j=1:max(size(rates))
        prRates(j,i) = sum(dataRate == rates(j))/max(size(dataRate));
    end
end

save('params.mat','prRates','rates');

