clear all;
close all;
curDir = pwd;
ii=0;
jj = 0;
font_size = 28;
marker_size = 14;
for nsta=[2 4 6 8 10 12 14 16 18 20 22 24]
    nsta
    ii = ii+1;
    dmax = 7;
    dvp = 0.01;
    
    str = sprintf('plots/pid_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp)
    cd(str);
    base = str(find(str == '/',1, 'last')+1:end)
    matFile = sprintf('%s.mat',base)
    load(matFile);%load all variables
    cd(curDir);
    
   jj =jj+2;
    if (jj<25)
        n_sta(ii) = jj;
    end
    
    util_pid(ii) = mean(avgBusy)/100;
    avg_delay_pid(ii) = mean(avgQWait{1}); %Caution! uses first station as sample for average queue waiting time
    avg_dvp_pid(ii) = mean(avgQDvp{1}); %uses maximum of all stations
    for jj=2:nsta
        avg_dvp_pid(ii) = max(avg_dvp_pid(ii),mean(avgQDvp{jj})); 
    end
    avg_dvp_pid(ii) = max(avg_dvp_pid(ii),eps);
    
    str = sprintf('plots/deadline_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp)
    cd(str);
    base = str(find(str == '/',1, 'last')+1:end)
    matFile = sprintf('%s.mat',base)
    load(matFile);%load all variables
    cd(curDir);
    util_deadline(ii) = mean(avgBusy)/100;
    avg_delay_deadline(ii) = mean(avgQWait{1}); %Caution! uses first station as sample for average queue waiting time
    avg_dvp_deadline(ii) = mean(avgQDvp{1}); %uses maximum of all stations
    for jj=2:nsta
        avg_dvp_deadline(ii) = max(avg_dvp_deadline(ii),mean(avgQDvp{jj})); 
    end
    avg_dvp_deadline(ii) = max(avg_dvp_deadline(ii),eps);
 
    str = sprintf('plots/edf_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp)
    cd(str);
    base = str(find(str == '/',1, 'last')+1:end)
    matFile = sprintf('%s.mat',base)
    load(matFile);%load all variables
    cd(curDir);
    util_edf(ii) = mean(avgBusy)/100;
    avg_delay_edf(ii) = mean(avgQWait{1}); %Caution! uses first station as sample for average queue waiting time
    avg_dvp_edf(ii) = mean(avgQDvp{1}); %uses maximum of all stations
    for jj=2:nsta
        avg_dvp_edf(ii) = max(avg_dvp_edf(ii),mean(avgQDvp{jj})); 
    end
    avg_dvp_edf(ii) = max(avg_dvp_edf(ii),eps);
 
end

figure;
plot(n_sta,util_pid,'ks-','MarkerSize',marker_size,'LineWidth',2);
grid;
ylim([0 1.01]);
hold on;
plot(n_sta,util_deadline,'ko-','MarkerSize',marker_size,'LineWidth',2);
plot(n_sta,util_edf,'kx-','MarkerSize',marker_size,'LineWidth',2);
xlabel('Number of Stations','fontsize',font_size);
ylabel('Channel Utilization','fontsize',font_size);
xlim([1 24]);
set(gca,'fontsize',font_size);

legend('PID Control','Deadline','EDF','Location','NorthWest');

figure;
plot(n_sta,avg_delay_pid/1000,'ks-','MarkerSize',marker_size,'LineWidth',2);
grid;
hold on;
plot(n_sta,avg_delay_deadline/1000,'ko-','MarkerSize',marker_size,'LineWidth',2);
plot(n_sta,avg_delay_edf/1000,'kx-','MarkerSize',marker_size,'LineWidth',2);
xlabel('Number of Stations','fontsize',font_size);
ylabel('Average End to End Delay (Seconds)','fontsize',font_size);
xlim([1 24]);
set(gca,'fontsize',font_size);
legend('PID Control','Deadline','EDF');

figure;
semilogy(n_sta,avg_dvp_pid,'ks-','MarkerSize',marker_size,'LineWidth',2);
grid;
hold on;
semilogy(n_sta,avg_dvp_deadline,'ko-','MarkerSize',marker_size,'LineWidth',2);
semilogy(n_sta,avg_dvp_edf,'kx-','MarkerSize',marker_size,'LineWidth',2);
ylim([0.001 1]);
xlim([1 24]);
line([1 24],[dvp dvp],'LineWidth',2);
xlabel('Number of Stations','fontsize',font_size);
ylabel('Delay Violation Probability','fontsize',font_size);
set(gca,'fontsize',font_size);
legend('PID Control','Deadline','EDF','Location','NorthWest');
