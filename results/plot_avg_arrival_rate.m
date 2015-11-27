% this script plots a moving average of the traffic generation (arrival)
% rate for the video source. The length of the moving average interval is
% set by the vaiable <interval>. 
% Our PID control aggregation algorithm will perform best when the arrival
% rate is close to CBR.
% you first have to create sent-bytes.txt from the video output file using
% the following bash command:
% cat sender-output0 | cut -d' ' -f1,28 > sent-bytes.txt
clear all;
close all;
data=load('sent-bytes.txt');
interval = 5;
times = sort(data(:,1));
j=find(times >= times(1)+interval);
for i = j(1):size(times) %start interval seconds after the start time
    end_time=times(i,1);
    start_time = max(times(1),end_time-interval); %consider last <interval> seconds
    index_mask = find((data(:,1) >= start_time) .* (data(:,1)<=end_time)); %mask the interval
    interval_bits = sum(data(index_mask,2))*8;
    avg_rate(i-j(1)+1,1) = end_time;
    avg_rate(i-j(1)+1,2) = interval_bits / (end_time-start_time);
end
plot(avg_rate(:,1),avg_rate(:,2)/1e6);
grid;
xlabel('Time (sec)');
ylabel('Avg. Arrival Rate (Mbps)');