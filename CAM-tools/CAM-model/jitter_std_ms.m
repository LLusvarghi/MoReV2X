function jitter = jitter_std_ms(scenario, profile)

switch profile
    case 'volkswagen'
        switch scenario
            case 'highway'
                jitter = 3.235;
            case 'suburban'
                jitter = 3.814;
            case 'urban'
                jitter = 3.444;
            case 'universal'
                jitter = 3.553;
            otherwise
                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
        end
    case 'renault'
        switch scenario
            case 'highway'
                jitter = 2.817;
            case 'suburban'
                jitter = 2.769;
            case 'urban'
                jitter = 2.711;
            case 'universal'
                jitter = 2.783;
            otherwise
                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
        end
    otherwise
        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
end

end