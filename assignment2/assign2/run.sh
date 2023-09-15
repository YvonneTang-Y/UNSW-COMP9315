rm ./data/*
make
# binary_file page_size buffer_slots max_opened_files buffer_replacement_policy database_folder input_data queries output_log

test_folder='test'

./main 64 6 3 CLS ./data ./$test_folder/test1/data_1.txt ./$test_folder/test1/query_1.txt ./$test_folder/test1/log_1.txt

# selection only test
./main 50 3 3 CLS ./data ./$test_folder/test2/data_2.txt ./$test_folder/test2/query_2.txt ./$test_folder/test2/log_2.txt 

# join only test （for enough buffers）
./main 50 14 2 CLS ./data ./$test_folder/test3/data_3.txt ./$test_folder/test3/query_3.txt ./$test_folder/test3/log_3.txt 

# join only test  
./main 50 5 2 CLS ./data ./$test_folder/test4/data_4.txt ./$test_folder/test4/query_4.txt ./$test_folder/test4/log_4.txt 

# mixed test 
./main 40 3 3 CLS ./data ./$test_folder/test5/data_5.txt ./$test_folder/test5/query_5.txt ./$test_folder/test5/log_5.txt 