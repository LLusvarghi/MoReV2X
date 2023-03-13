clear all;
%% Script to generate CAM intervals and sizes
% This script shows how to generate an arbitrary number of CAM sizes
% and intervals using the the markov chain model proposed in:
% Paper title: Empirical Models for the Realistic Generation of Cooperative Awareness Messages in Vehicular Networks
% Authors: Rafael Molina-Masegosa, Miguel Sepulcre, Javier Gozalvez, Friedbert Berens and Vincent Martinez

%% Parameters

% number of CAMs that will be generated
n_cam = 1e5;

% scenario ('highway', 'suburban', 'urban' or 'universal')
scenario = 'highway';

% car manufacturer profile ('volkswagen' or 'renault')
profile = 'volkswagen';

% generate complete model (intervals+sizes), only intervals, or only sizes
% (use 'complete', 'intervals' or 'sizes')
model = 'complete';

% Parameter m: number of states, including present state and previous ones, 
% which are taken into account to calculate the transition to the new state
% (valid values for m are 1 and 5)
m = 5;

%% Load the transition matrix M and PDF 
% Load the transition matrix M and the PDF for the corresponding traces,
% depending on the model, scenario and profile selected
[M,pdf] = load_M_PDF(scenario, profile, model, m);

% Number of possible sequences of 'm' symbols in the Markov chain model,
% 'N'
N = size(pdf,1);

% Number of possible sizes |S| and intervals |G|

switch profile
    case 'volkswagen'
        S = 4;
    case 'renault'
        S = 5;
    otherwise
        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
end

G = 10;


%% Generation

switch model
    case 'complete'
        % Arrays to store the generated sizes and intervals, where interval 
        % 'k' is the interval before CAM with size 'k+1' in the arrays
        CAMsizes = zeros(1,n_cam);
        CAMintervals = zeros(1,n_cam-1);  %For 'n' cams, there are 'n-1' intervals
        
        % Array to store symbols [symbol = (interval - 1)*S + size]
        CAMsymbols = zeros(1,n_cam);
        
        %Arrays to plot results at the end
        CAMsizesIndex = zeros(1,n_cam);
        hist_corr = zeros(10,G*S/10);

        % First 'm' messages: random symbol following PDF
        r = rand;
        for i = 1:N
            if r <= sum(pdf(1:i,m+1))
                current_symbols = pdf(i,1:m);
                break;
            end
        end
        
        for i = 1:m
            CAMsymbols(i) = current_symbols(i);
            CAMsizes(i) = sizeBytes(profile,mod(current_symbols(i)-1,G*S/10)+1);
            CAMsizesIndex(i) = mod(current_symbols(i)-1,G*S/10)+1;
            if i > 1
                CAMintervals(i-1) = (floor((current_symbols(i)-1)/(G*S/10))+1) * 100 + normrnd(0,jitter_std_ms(scenario,profile));
            end
        end
        
        % Following traces: use the model
        for k = (m+1):n_cam            
            % Look for the sequence of 'm' last symbols in matrix 'M'
            [LIA,LOCB] = ismember(M(:,1:m),CAMsymbols((k-m):(k-1)), 'rows');
            sequence = find(LOCB);
            
            % Probabilities of transition to the next symbol
            P_trans = M(sequence,m+2);
            
            % Select next symbol
            x = find(cumsum(P_trans) >= rand);
            next_symbol = M(sequence(x(1)),m+1);
            
            % Store symbol
            CAMsymbols(k) = next_symbol;
            % Store size
            CAMsizes(k) = sizeBytes(profile,mod(next_symbol-1,G*S/10)+1);
            CAMsizesIndex(k) = mod(next_symbol-1,G*S/10)+1;
            %Store interval in miliseconds and add the jitter
            CAMintervals(k-1) = (floor((next_symbol-1)/(G*S/10))+1) * 100 + normrnd(0,jitter_std_ms(scenario,profile));
            
            hist_corr((floor((next_symbol-1)/(G*S/10))+1),CAMsizesIndex(k)) = hist_corr((floor((next_symbol-1)/(G*S/10))+1),CAMsizesIndex(k)) + 1;
        end
        
        %correlated interval-size PDF
        figure
        bar(hist_corr./sum(hist_corr(:)));
        xlabel('CAM time interval (x100 ms)');
        ylabel('PDF');
        switch profile
            case 'volkswagen'
                legend('200 bytes','300 bytes','360 bytes','455 bytes');                
            case 'renault'
                legend('200 bytes','330 bytes','480 bytes','600 bytes','800 bytes'); 
            otherwise
                error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
        end
        
        %jitter PDF
        figure
        histogram(CAMintervals - (round(CAMintervals/100)*100),-10.5:1:10.5,'Normalization','probability');
        ylabel('PDF');
        xlabel('CAM time interval jitter (ms)');   
        
    case 'intervals'
        % Array to store the generated intervals
        CAMintervals = zeros(1,n_cam-1);  %For 'n' cams, there are 'n-1' intervals   
        
        % Array to store symbols [symbol = interval(ms) / 100]
        CAMsymbols = zeros(1,n_cam-1);
        
        % First 'm' messages: random state following PDF
        r = rand;
        for i = 1:N
            if r <= sum(pdf(1:i,m+1))
                current_symbols = pdf(i,1:m);
                break;
            end
        end
        
        CAMsymbols(1:m) = current_symbols(1:m);
        CAMintervals(1:m) = current_symbols(1:m) * 100;
        

        % Following traces: use the model
        for k = (m+1):(n_cam-1)
            % Look for the sequence of 'm' last symbols in matrix 'M'
            [LIA,LOCB] = ismember(M(:,1:m),CAMsymbols((k-m):(k-1)), 'rows');
            sequence = find(LOCB);
            
            % Probabilities of transition to the next symbol
            P_trans = M(sequence,m+2);
            
            % Select next symbol
            x = find(cumsum(P_trans) >= rand);
            next_symbol = M(sequence(x(1)),m+1);
            
            % Store symbol
            CAMsymbols(k) = next_symbol;
            %Store interval in miliseconds and add the jitter
            CAMintervals(k) = next_symbol * 100 + normrnd(0,jitter_std_ms(scenario,profile));
        end
        
        %intervals PDF
        figure
        histogram(CAMintervals,50:100:1050,'Normalization','probability');
        ylabel('PDF');
        xlabel('CAM time interval (ms)');
        
        %jitter PDF
        figure
        histogram(CAMintervals - (round(CAMintervals/100)*100),-10.5:1:10.5,'Normalization','probability');
        ylabel('PDF');
        xlabel('CAM time interval jitter (ms)');
        
    case 'sizes'
        % Array to store the generated sizes
        CAMsizes = zeros(1,n_cam);  
        
        % Array to store symbols
        CAMsymbols = zeros(1,n_cam-1);
        
        % First 'm' messages: random state following PDF
        r = rand;
        for i = 1:N
            if r <= sum(pdf(1:i,m+1))
                current_symbols = pdf(i,1:m);
                break;
            end
        end
        
        CAMsymbols(1:m) = current_symbols(1:m);        
        for i=1:m
            CAMsizes(i) = sizeBytes(profile,current_symbols(i));
        end
        
        % Following traces: use the model
        for k = (m+1):(n_cam-1)
            % Look for the sequence of 'm' last symbols in matrix 'M'
            [LIA,LOCB] = ismember(M(:,1:m),CAMsymbols((k-m):(k-1)), 'rows');
            sequence = find(LOCB);
            
            % Probabilities of transition to the next symbol
            P_trans = M(sequence,m+2);
            
            % Select next symbol
            x = find(cumsum(P_trans) >= rand);
            next_symbol = M(sequence(x(1)),m+1);
            
            % Store symbol
            CAMsymbols(k) = next_symbol;
            %Store size in bytes
            CAMsizes(k) = sizeBytes(profile,next_symbol);
        end
        
        figure
        histogram(CAMsizes,'Normalization','probability');
        ylabel('PDF');
        xlabel('CAM size (bytes)');
        
    otherwise
        error('''model'' parameter value is not valid. Try ''complete'', ''intervals'' or ''sizes''');
end

% clean auxiliary variables
clear CAMsizesIndex current_state hist_corr i k M model N profile r scenario pdf LIA LOCB P_trans S sequence x next_symbol G current_symbols n_cam