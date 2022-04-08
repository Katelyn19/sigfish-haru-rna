#!/bin/bash

set -x
read_id=$1

slow5tools get --to slow5 test/sequin_reads.blow5 $1 | grep -v '^[#@]' | awk '{print $8}' > $1.txt
./sigfish event --rna test/sequin_reads.blow5 $1 -n | awk '{print $3"\t"$4"\t"$5}' > $1.events.txt
./sigfish seg test/sequin_reads.blow5 $1 -n | cut -f 3,4,8,9 > $1.adaptor.txt

matlab.exe -nodisplay -nosplash -nodesktop -r "
a=dlmread('$1.txt'); b=dlmread('$1.events.txt');
startidx=b(:,1)+1; endidx=b(:,2)+startidx-1;
avg=zeros(length(a),1);
for j=1:length(startidx)
    avg(startidx(j):endidx(j))=mean(a(startidx(j):endidx(j)));
end
x=dlmread('$1.adaptor.txt');
x1=x(:,[1:2]);
x2=x(:,[3:4]);
y=[1200,1200];
plot(a); hold on; plot(avg); xlabel('sample index'), ylabel('raw signal value'); stem(x1,y); stem(x2,y); legend('raw signal','events','jnn-adaptor','jnn-polya'); savefig('$1.fig');

c=dlmread('$1.adaptor.txt');

%figure; plot(b(:,3)); savefig('$1.events.fig');
"

