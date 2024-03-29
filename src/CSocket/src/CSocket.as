package {

	import com.curi.net.ChatSocket;

	import flash.display.Sprite;
	import flash.external.ExternalInterface;

	public class CSocket extends Sprite {

		private var _chat:ChatSocket;

		public function CSocket() {
			_chat = new ChatSocket( '127.0.0.1', 11223 );
			ExternalInterface.addCallback( 'join', join );
			ExternalInterface.addCallback( 'sndMsg', sendMsg );
			ExternalInterface.addCallback( 'leave', leave );
		}

		private function join( name:String ):void {
			_chat.join( name );
		}

		private function leave():void {
			_chat.leave();
		}

		private function sendMsg( body:String ):void {
			_chat.sendMessage( body );
		}

		private function debugTrace( ... args ):void {
			var str:String = args.join( ' ' );
			trace( str );
			ExternalInterface.call( 'console.log', str );
		}
	}
}
