clear all;
close all;
curDir = pwd;
kk=0;
util = figure;
ta = figure;
pattern0 = {'ks-','ko-','kx-','k*-','kd-','k^-','kv-'};
font_size = 28;
marker_size = 14;

for nsta=[14]
    ii=0;
    kk=kk+1;
    lgnd{kk} = sprintf('%d STA(s)',nsta);
    for dmax=[5]
        ii = ii+1;
        dvp = 0.01;
        str = sprintf('plots/deadline_nSta%d_dMax%d_dvp%.2f',nsta,dmax,dvp);
	generate_plots(str);
    end;
end;
