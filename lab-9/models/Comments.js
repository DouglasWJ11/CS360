var mongoose = require('mongoose');
var CommentSchema = new mongoose.Schema({
  title: String,
  upvotes: {type: Number, default: 0},
});
mongoose.model('Comment', CommentSchema);

CommentSchema.methods.upvote=function(cb){
	this.upvotes+=1;
	this.save(cb);
}
CommentSchema.methods.downvote=function(cb){
	this.upvotes -=1;
	this.save(cb);
}
