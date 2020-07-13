#include"btree_header.h"
#include<stdio.h>
#include<stdlib.h>
static void inorder_traversal(FILE *fp, int root_offset);
static void preorder_traversal(FILE *fp, int root_offset);
static int search(FILE *fp, int key);

// create a file with the filename and initialize the header with tree_t structure
// if the file already exists, just return the file ptr
FILE* init_tree(const char* filename)
{
	FILE* fp;
	fp = fopen(filename,"r+b"); //Opening in the r+b mode initially
	if(fp==NULL) //If the given file is not present
	{
		fp = fopen(filename,"w+b"); //Creating a new file with the w+b mode
		tree_t header; //Header of the tree
		header.free_head = 0; //Initialising the header to the free list
		header.root = -1; //Initialising the root (-1 indicates NULL)
		fseek(fp,0,SEEK_SET);
		fwrite(&header,sizeof(tree_t),1,fp); //Writing the header structure to the beginning of the file
	}


	return fp;
}

// close the file pointed by fp
void close_tree(FILE *fp)
{
	fclose(fp);
}

// Space separated keys. Last key followed by '\n'
void display_inorder(FILE *fp)
{
	tree_t head; //Header of the tree
	fseek(fp,0,SEEK_SET);
	fread(&head,sizeof(tree_t),1,fp); //Reading the header of the tree
	if(head.root!=-1) //If the root node is present
	{
		inorder_traversal(fp, head.root); //Recursive function call starting with the tree's root node
	}
	printf("\n");
}

// Space separated keys. Last key followed by '\n'
void display_preorder(FILE *fp)
{
	tree_t head; //Header of the tree
	fseek(fp,0,SEEK_SET);
	fread(&head,sizeof(tree_t),1,fp); //Reading the header of the tree
	if(head.root!=-1) //If the root node is present
	{
		preorder_traversal(fp, head.root); //Recursive function call starting with the tree's root node
	}
	printf("\n");

}

// insert the key into the tree in the file
// if the key already exists just return
void insert_key(int key, FILE *fp)
{
	if(search(fp,key)) //If the key is already present in the tree
		return;
	int insert_offset; //Offset at which the new key's node should be inserted
	tree_t head; //Header of the tree
	node_t temp; //New node
	temp.key = key; //Initialising the key
	temp.left_offset = -1; //Grounding the left and right offsets to -1
	temp.right_offset = -1;
	fseek(fp,0,SEEK_SET);
	fread(&head,sizeof(tree_t),1,fp);
	insert_offset = head.free_head; //Offset for the new node

	fseek(fp,sizeof(tree_t)+sizeof(node_t)*head.free_head,SEEK_SET); //Updating the free_head offset
	if(getc(fp)==EOF) //If free_head is already pointing to the end of the file, just incrementing
		head.free_head = head.free_head+1;
	else //If free_head is pointing to some node in the middle of the file(free_head is pointing to the list of free nodes)
	{	
		int free_temp;
		node_t free_node;
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*head.free_head,SEEK_SET);
		fread(&free_node,sizeof(node_t),1,fp); //Reading the node pointed by the free_head offset
		free_temp = head.free_head;
		head.free_head = free_node.right_offset; //Changing the free_head offset to the next offset which is free
		free_node.right_offset = -1; //Grounding the right offset of the node after updating free_head
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*free_temp,SEEK_SET);
		fwrite(&free_node,sizeof(node_t),1,fp); //Writing back that node to the file

	}

	if(head.root==-1) //If the tree is empty
	{	
		//Making the new node as the root node of the tree
		head.root = insert_offset; //Updating the tree's root offset
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*insert_offset,SEEK_SET);
		fwrite(&temp,sizeof(node_t),1,fp); //Writing the new key's node at the respective offset in the file
	}
	else //If the tree is not empty
	{
		int prev = -1; //Offset for the parent
		int cur = head.root; //Offset for traversal
		node_t cur_node;
		while(cur!=-1) //Until a leaf node is reached
		{	
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
			fread(&cur_node,sizeof(node_t),1,fp); //Current node in the traversal
			prev = cur; //Updating the parent

			if(cur_node.key < key) //If the key to be inserted is greater than the current node's key, traverse to the right
				cur = cur_node.right_offset;
			else //Else, if the key to be inserted is lesser than the current node's ley, traverse to the left
				cur = cur_node.left_offset;
		}
		node_t prev_node;
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*prev,SEEK_SET);
		fread(&prev_node,sizeof(node_t),1,fp); //Read the parent node for which the new node has to be inserted as a child

		if(prev_node.key < key) //If the parent's key is less than the key to be inserted, insert it as the right child
			prev_node.right_offset = insert_offset;
		else //If the parent's key is greater than the key to be inserted, insert it as the left child
			prev_node.left_offset = insert_offset;

		fseek(fp,sizeof(tree_t)+sizeof(node_t)*prev,SEEK_SET);
		fwrite(&prev_node,sizeof(node_t),1,fp); //Writing the parent node after updating back to the file

		fseek(fp,sizeof(tree_t)+sizeof(node_t)*insert_offset,SEEK_SET);
		fwrite(&temp,sizeof(node_t),1,fp); //Writing the new node to the file at its respective insert_offset
	}
	fseek(fp,0,SEEK_SET);
	fwrite(&head,sizeof(tree_t),1,fp); //Writing the header of the tree back to the file
}

// delete the key from the tree in the file
// the key may or may not exist
void delete_key(int key, FILE *fp)
{
	tree_t head; //Header of the tree
	node_t cur_node;
	fseek(fp,0,SEEK_SET);
	fread(&head,sizeof(tree_t),1,fp); //Reading the header
	if(head.root==-1) //If the tree is empty
		return;
	int prev,cur; //Offset for the parent
	prev = -1;
	cur = head.root; //Starting the traversal from the root
	fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
	fread(&cur_node,sizeof(node_t),1,fp);
	while(cur!=-1 && cur_node.key!=key) //Until the complete tree is traversed or the key to be deleted is found
	{
		prev = cur;
		if(key < cur_node.key) //Traversing to the left 
			cur = cur_node.left_offset;
		else //Traversing to the right
			cur = cur_node.right_offset;
		if(cur!=-1)
		{
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
			fread(&cur_node,sizeof(node_t),1,fp); 
		}
	}
	
	if(cur==-1) //Complete tree is traversed and the key is not found
		return;
	
	fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
	fread(&cur_node,sizeof(node_t),1,fp); //Reading the current node whose key has to be deleted

	if(cur_node.left_offset==-1 || cur_node.right_offset==-1) //If either of the subtrees are not present
	{	
		int in_offset; //Offset to the  inorder predecessor/successor
		if(cur_node.left_offset==-1) //If the left subtree is empty, successor is the right subtree
			in_offset = cur_node.right_offset;
		else //Else, the left subtree is the predecessor
			in_offset = cur_node.left_offset;

		if(prev==-1) //The root should be deleted
		{
			head.root = in_offset; //Successor/Predecessor made as the root
		}
		else
		{
			node_t prev_node;
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*prev,SEEK_SET);
			fread(&prev_node,sizeof(node_t),1,fp); //Reading the parent node

			if(prev_node.left_offset == cur) //Attaching the inorder predecessor to the parent node
				prev_node.left_offset = in_offset;
			else //Attaching the inorder successor to the parent node
				prev_node.right_offset = in_offset;

			fseek(fp,sizeof(tree_t)+sizeof(node_t)*prev,SEEK_SET);
			fwrite(&prev_node,sizeof(node_t),1,fp); //Writing back the parent node to the file after updating
		}
		//Attaching the current node to the free list pointed by the free_head offset
		cur_node.right_offset = head.free_head;
		cur_node.left_offset = -1;
		head.free_head = cur; //Updating the free_head offset
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
		fwrite(&cur_node,sizeof(node_t),1,fp); //Writing back the current node to the file, after adding it to the list of free nodes
	}
	else //Both the subtrees are present for the node whose key needs to be deleted
	{	
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
		fread(&cur_node,sizeof(node_t),1,fp);
		int parent; //Parent to the inorder successor of the current node
		int temp; //Offset of the inorder successor of the current node
		node_t temp_node;
		parent = -1;
		temp = cur_node.right_offset; 
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*temp,SEEK_SET);
		fread(&temp_node,sizeof(node_t),1,fp);
		while(temp_node.left_offset!=-1) //Finding the inorder successor which is the leftmost node in the right subtree of the current node
		{
			parent = temp;
			temp = temp_node.left_offset;
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*temp,SEEK_SET);
			fread(&temp_node,sizeof(node_t),1,fp);
		}

		if(parent!=-1) //If the inorder successor is not the current node's immediate right child node
		{
			node_t parent_node;
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*parent,SEEK_SET);
			fread(&parent_node,sizeof(node_t),1,fp); //Reading the parent node
			parent_node.left_offset = temp_node.right_offset; //The inorder successor's right child is made as the parent's left child 
			fseek(fp,sizeof(tree_t)+sizeof(node_t)*parent,SEEK_SET);
			fwrite(&parent_node,sizeof(node_t),1,fp); //Writing back the parent node to the file
		}
		else //If the inorder successor is the current node's immediate right child
		{
			cur_node.right_offset = temp_node.right_offset; //The successor's right child is made as the current node's right child
		}
		cur_node.key = temp_node.key; //Current node's key value is replaced by the inorder successor's key value
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
		fwrite(&cur_node,sizeof(node_t),1,fp); //Writing back the current node to the file
		//Attaching the inorder successor's node to the list of free nodes
		temp_node.right_offset = head.free_head;
		temp_node.left_offset = -1;
		head.free_head = temp; //Updating the offset of free_head
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*temp,SEEK_SET);
		fwrite(&temp_node,sizeof(node_t),1,fp); //Writing back the successor node to the file, after attaching it to the list of free nodes
	}

	fseek(fp,0,SEEK_SET);
	fwrite(&head,sizeof(tree_t),1,fp); //Writing back the header of the tree to the file
	
}

static void inorder_traversal(FILE *fp, int root_offset)
{
	node_t root;
	if(root_offset!=-1) //If the root is present
	{	
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*root_offset,SEEK_SET);
		fread(&root,sizeof(node_t),1,fp); //Read the root node
		inorder_traversal(fp,root.left_offset); //Recursive call for the left sub-tree of the present root node
		printf("%d ",root.key); //Printing the root node's key
		inorder_traversal(fp,root.right_offset); //Recursive call for the right sub-tree of the present root node
	}
	return;
}

static void preorder_traversal(FILE *fp, int root_offset)
{
	node_t root;
	if(root_offset!=-1) //If the root is present
	{
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*root_offset,SEEK_SET);
		fread(&root,sizeof(node_t),1,fp); //Read the root node
		printf("%d ",root.key); //Printing the root node's key
		preorder_traversal(fp,root.left_offset); //Recursive call for the left sub-tree of the present root node
		preorder_traversal(fp,root.right_offset); //Recursive call for the right sub-tree of the present root node
	}
	return;
}

//Function to search for a given key in the tree
//Returns 1 if the key is found in the tree, else returns 0 if the key is not found
static int search(FILE *fp, int key)
{
	tree_t head;
	node_t cur_node; 
	fseek(fp,0,SEEK_SET);
	fread(&head,sizeof(tree_t),1,fp);
	int cur = head.root; //Starting the traversal from the root node
	while(cur!=-1)
	{	
		fseek(fp,sizeof(tree_t)+sizeof(node_t)*cur,SEEK_SET);
		fread(&cur_node,sizeof(node_t),1,fp); //Reading the current node in the traversal
		if(key>cur_node.key) //Traverse to the right if the search key is greater than the current node's key
			cur = cur_node.right_offset;
		else if(key<cur_node.key) //Traverse to the left if the search key is lesser than the current node's key
			cur = cur_node.left_offset;
		else //Current node's key is equal to the search key, the key is found in the tree
			return 1;
	}
	return 0;
}
