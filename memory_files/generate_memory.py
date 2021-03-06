import numpy as np

max_num_loops = 200

for dim in range(1,51):
    memChol = []
    ansMemChol = []

    for i in range(max_num_loops):
        A = np.random.randint(low=1,high=100,size=dim*dim)
        AAT = (A.reshape((dim,dim)) @ A.reshape((dim,dim)).T).flatten()
        L = (np.linalg.cholesky(AAT.reshape(dim,dim))).flatten()
        for j in AAT:
            memChol.append(j)
        for j in L:
            ansMemChol.append(j)

    np_memChol = np.array(memChol)
    np_ansMemChol = np.array(ansMemChol)

    np.savetxt("mem_text_" + str(dim) + "x" + str(dim) + ".txt", np_memChol, delimiter=",", newline=" ", fmt="%d")
    np.savetxt("cholesky_mem_text_" + str(dim) + "x" + str(dim) + ".txt", np_ansMemChol, delimiter=" ", newline=",", fmt="%f")
