%% Returns size in Bytes of the packet from the size index
function size = sizeBytes(profile, sizeIndex)
VolkswagenSizes = [200 300 360 455];
RenaultSizes = [200 330 480 600 800];

switch profile
    case 'volkswagen'
        size = VolkswagenSizes(sizeIndex);
    case 'renault'
        size = RenaultSizes(sizeIndex);
    otherwise
        error('''profile'' parameter value is not valid. Try ''volkswagen'' or ''renault''');
end
end