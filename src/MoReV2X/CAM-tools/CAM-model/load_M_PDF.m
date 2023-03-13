function [M,pdf] = load_M_PDF(scenario, profile, model, m)
switch m
    case 1
        switch model
            case 'complete'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            case 'intervals'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_IntervalsOnly_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_IntervalsOnly_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_IntervalsOnly_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_IntervalsOnly_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_IntervalsOnly_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_IntervalsOnly_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_IntervalsOnly_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_IntervalsOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_IntervalsOnly_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            case 'sizes'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_SizesOnly_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_SizesOnly_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_SizesOnly_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_SizesOnly_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_SizesOnly_m1.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_SizesOnly_m1.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_SizesOnly_m1.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_SizesOnly_m1.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_SizesOnly_m1.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            otherwise
                error('''model'' parameter value is not valid. Try ''complete'', ''intervals'' or ''sizes''');
        end
    case 5
        switch model
            case 'complete'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            case 'intervals'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_IntervalsOnly_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_IntervalsOnly_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_IntervalsOnly_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_IntervalsOnly_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_IntervalsOnly_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_IntervalsOnly_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_IntervalsOnly_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_IntervalsOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_IntervalsOnly_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            case 'sizes'
                switch profile
                    case 'volkswagen'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_VolkswagenHighway_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenHighway_SizesOnly_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_VolkswagenSuburban_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenSuburban_SizesOnly_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_VolkswagenUrban_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUrban_SizesOnly_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_VolkswagenUniversal_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_VolkswagenUniversal_SizesOnly_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    case 'renault'
                        switch scenario
                            case 'highway'
                                M = csvread('M_matrix\M_RenaultHighway_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultHighway_SizesOnly_m5.csv');
                            case 'suburban'
                                M = csvread('M_matrix\M_RenaultSuburban_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultSuburban_SizesOnly_m5.csv');
                            case 'urban'
                                M = csvread('M_matrix\M_RenaultUrban_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUrban_SizesOnly_m5.csv');
                            case 'universal'
                                M = csvread('M_matrix\M_RenaultUniversal_SizesOnly_m5.csv');
                                pdf = csvread('PDF\PDF_RenaultUniversal_SizesOnly_m5.csv');
                            otherwise
                                error('''scenario'' parameter value is not valid. Try ''highway'', ''suburban'', ''urban'' or ''universal''');
                        end
                    otherwise
                        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
                end
            otherwise
                error('''model'' parameter value is not valid. Try ''complete'', ''intervals'' or ''sizes''');
        end
        
    otherwise
        error('''m'' parameter value is not valid. Try 1 or 5');
end
end