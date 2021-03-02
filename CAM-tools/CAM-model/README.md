# Empirical Models for the Realistic Generation of Cooperative Awareness Messages in Vehicular Networks
This repository includes the models presented in the following paper:

    Rafael Molina-Masegosa, Miguel Sepulcre, Javier Gozalvez, Friedbert Berens, Vincent Martinez
    "Empirical Models for the Realistic Generation of Cooperative Awareness Messages in Vehicular Networks", 
    IEEE Transactions on Vehicular Technology (Early Access), DOI: 10.1109/TVT.2020.2979232
    https://ieeexplore.ieee.org/abstract/document/9027961
    https://arxiv.org/abs/2004.06919 

In order to comply with our sponsor guidelines, we would appreciate if any publication using this data references the above-mentioned publication.

The models are represented by a set of matrices that represent the conditional probabilities of the *m*th order Markov source models proposed. This set of matrices is in the folder called "M_matrix". In total, there are 48 models for:

- Renault and Volkswagen traces.
- Highway, suburban, urban and universal scenarios.
- Complete model, separate CAM time-intervals model and separate CAM sizes model.
- *m*=1 and *m*=5.

In addition, the folder called "PDF" includes the PDF (Probability Density Function) extracted from real traces, necessary to generate the first CAM of a trace when using the models. For the complete models, the joint size and time-interval PDF is provided, while only the CAM time-interval PDF and the CAM size PDFs are provided for the separate time-intervals models and separate sizes models respectively.

The Matlab script "traces_generation_example.m" provides an example of how to generate an arbitrary number of CAM time-intervals and sizes using each of the models.
